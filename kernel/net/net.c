#include <net/net.h>
#include <net/e1000.h>
#include <net/tcp.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <string.h>
#include <stdint.h>

/* 21A: Gerçek ethernet/IP yığını (e1000 üstünde, polling).
 * - Ethernet: 0x0806 ARP, 0x0800 IPv4 dağıtımı.
 * - ARP: cache + istek/yanıt.
 * - IPv4: ICMP echo (ping yanıtı) + UDP soket demux + TCP'ye geçirme.
 * Yerel yapılandırma QEMU slirp (10.0.2.15) ile eşleşir. */

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IP  0x0800
#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP  17
#define IP_PROTO_TCP  6

#define NET_BUF 1536

static uint8_t local_mac[6];
static uint32_t local_ip = 0x0A00020F; /* 10.0.2.15 */

#define ARP_CACHE 8
struct arp_entry {
    uint32_t ip;
    uint8_t mac[6];
    int used;
};
static struct arp_entry arp_cache[ARP_CACHE];
static uint8_t arp_count_ev = 0;

static uint16_t ip_id_seq = 0;

struct udp_sock {
    int used;
    uint16_t lport;
    uint16_t rport;
    uint32_t rip;
    uint8_t rbuf[2048];
    int rlen;
};
#define UDP_MAX_SOCK 4
static struct udp_sock udp_socks[UDP_MAX_SOCK];

static uint8_t g_frame[NET_BUF];

static uint16_t get16(const uint8_t* p) { return ((uint16_t)p[0] << 8) | p[1]; }
static uint32_t get32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static void put16(uint8_t* p, uint16_t v) { p[0] = v >> 8; p[1] = v & 0xFF; }
static void put32(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF; p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF; p[3] = v & 0xFF;
}

static uint16_t ones_sum(const uint8_t* p, int len) {
    uint32_t s = 0;
    while (len > 1) {
        s += ((uint16_t)p[0] << 8) | p[1];
        p += 2; len -= 2;
    }
    if (len) s += (uint16_t)p[0] << 8;
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)s;
}

static uint16_t ip_cksum(const uint8_t* h) {
    return (uint16_t)~ones_sum(h, 20);
}

static uint8_t* arp_lookup(uint32_t ip) {
    for (int i = 0; i < ARP_CACHE; i++)
        if (arp_cache[i].used && arp_cache[i].ip == ip) return arp_cache[i].mac;
    return 0;
}

static void arp_add(uint32_t ip, const uint8_t* mac) {
    for (int i = 0; i < ARP_CACHE; i++) {
        if (arp_cache[i].used && arp_cache[i].ip == ip) {
            for (int k = 0; k < 6; k++) arp_cache[i].mac[k] = mac[k];
            return;
        }
    }
    int s = arp_count_ev++ % ARP_CACHE;
    arp_cache[s].ip = ip;
    for (int k = 0; k < 6; k++) arp_cache[s].mac[k] = mac[k];
    arp_cache[s].used = 1;
}

/* Ethernet + ARP isteği kur ve gönder. ip: hedef IP (ağ sırası). */
static void arp_request(uint32_t ip) {
    uint8_t f[42];
    for (int i = 0; i < 6; i++) f[i] = 0xFF;                 /* dst braodcast */
    for (int i = 0; i < 6; i++) { f[6 + i] = local_mac[i]; f[32 + i] = local_mac[i]; }
    put16(f + 12, ETH_TYPE_ARP);
    put16(f + 14, 1); put16(f + 16, ETH_TYPE_IP);
    f[18] = 6; f[19] = 4;
    put16(f + 20, 1);                  /* request */
    for (int i = 0; i < 6; i++) f[22 + i] = local_mac[i];
    put32(f + 28, local_ip);
    for (int i = 0; i < 6; i++) f[32 + i] = 0;               /* hedef MAC boş */
    put32(f + 38, ip);
    e1000_send(f, sizeof(f));
}

static void arp_reply(const uint8_t* dst_mac, uint32_t dst_ip) {
    uint8_t f[42];
    for (int i = 0; i < 6; i++) { f[i] = dst_mac[i]; f[6 + i] = local_mac[i]; }
    put16(f + 12, ETH_TYPE_ARP);
    put16(f + 14, 1); put16(f + 16, ETH_TYPE_IP);
    f[18] = 6; f[19] = 4;
    put16(f + 20, 2);                  /* reply */
    for (int i = 0; i < 6; i++) f[22 + i] = local_mac[i];
    put32(f + 28, local_ip);
    for (int i = 0; i < 6; i++) f[32 + i] = dst_mac[i];
    put32(f + 36, dst_ip);
    e1000_send(f, sizeof(f));
}

static void arp_handle(const uint8_t* p, int len) {
    if (len < 28) return;
    uint16_t hw = get16(p), proto = get16(p + 2);
    if (hw != 1 || proto != ETH_TYPE_IP) return;
    uint32_t spa = get32(p + 14);   /* sender ip */
    uint32_t tpa = get32(p + 24);   /* target ip */
    const uint8_t* smac = p + 8;
    /* öğren */
    arp_add(spa, smac);
    uint16_t op = get16(p + 6);
    if (op == 1 && tpa == local_ip) {
        arp_reply(smac, spa);
    }
}

/* IPv4 çerçevesi ekle (ip + veri), checksum hesapla. dst_mac 0 ise ARP çöz. */
static int ip_sendframe(uint32_t dst_ip, uint8_t proto, const uint8_t* data, int dlen,
                        uint16_t sport, uint16_t dport, int is_udp) {
    uint8_t* mac = arp_lookup(dst_ip);
    if (!mac) {
        arp_request(dst_ip);
        return -1; /* ARP çözümlemeden gönderilemez (test ilk ARP sonrası dener) */
    }
    int ih = 20;
    int total = 14 + ih + dlen;
    if (total > NET_BUF) return -1;
    uint8_t* f = g_frame;
    for (int i = 0; i < 6; i++) { f[i] = mac[i]; f[6 + i] = local_mac[i]; }
    put16(f + 12, ETH_TYPE_IP);
    uint8_t* ip = f + 14;
    ip[0] = 0x45; ip[1] = 0;
    put16(ip + 2, (uint16_t)(ih + dlen));
    put16(ip + 4, ip_id_seq++);
    put16(ip + 6, 0x4000);          /* DF */
    ip[8] = 64; ip[9] = proto;
    put16(ip + 10, 0);
    put32(ip + 12, local_ip);
    put32(ip + 16, dst_ip);
    put16(ip + 10, ip_cksum(ip));
    for (int i = 0; i < dlen; i++) ip[ih + i] = data[i];
    (void)sport; (void)dport; (void)is_udp;
    return e1000_send(f, total);
}

/* ICMP (echo) — paketin kaynak IP'sini bilmek gerekir */
static uint16_t icmp_ping_id = 0;
static int icmp_ping_reply = 0;

void net_icmp_send(uint32_t dst_ip) {
    uint8_t d[32];
    d[0] = 8; d[1] = 0;               /* echo request */
    put16(d + 2, 0);
    icmp_ping_id = (uint16_t)(0xBEEF + ip_id_seq);
    put16(d + 4, icmp_ping_id); put16(d + 6, 0);
    for (int i = 8; i < 32; i++) d[i] = (uint8_t)(0x11 + i);
    uint16_t s = ones_sum(d, 32);
    put16(d + 2, (uint16_t)~s);
    ip_sendframe(dst_ip, IP_PROTO_ICMP, d, sizeof(d), 0, 0, 0);
}

int net_icmp_hit(void) { return icmp_ping_reply; }
void net_icmp_reset(void) { icmp_ping_reply = 0; }

static void icmp_handle(uint32_t src_ip, const uint8_t* p, int len) {
    if (len < 8) return;
    if (p[0] == 8) {                 /* echo request -> reply */
        uint8_t d[64];
        int n = len > 64 ? 64 : len;
        memcpy(d, p, n);
        d[0] = 0;                    /* reply */
        put16(d + 2, 0);
        uint16_t s = ones_sum(d, n);
        put16(d + 2, (uint16_t)~s);
        ip_sendframe(src_ip, IP_PROTO_ICMP, d, n, 0, 0, 0);
        return;
    }
    if (p[0] == 0 && get16(p + 4) == icmp_ping_id) /* echo reply */
        icmp_ping_reply = 1;
}

/* UDP soket API'si (21A testi için minimal) */
int udp_sock_open(uint16_t lport) {
    for (int i = 0; i < UDP_MAX_SOCK; i++) {
        if (!udp_socks[i].used) {
            udp_socks[i].used = 1;
            udp_socks[i].lport = lport;
            udp_socks[i].rport = 0;
            udp_socks[i].rip = 0;
            udp_socks[i].rlen = 0;
            return i;
        }
    }
    return -1;
}

int udp_sock_send(int s, uint32_t dst_ip, uint16_t dport, const uint8_t* data, int len) {
    if (s < 0 || s >= UDP_MAX_SOCK || !udp_socks[s].used) return -1;
    uint8_t d[1500];
    if (len > 1472) len = 1472;
    uint8_t* uh = d;
    put16(uh, udp_socks[s].lport); put16(uh + 2, dport);
    put16(uh + 4, (uint16_t)(8 + len));
    put16(uh + 6, 0);
    for (int i = 0; i < len; i++) uh[8 + i] = data[i];
    /* pseudo başlık: src(4) dst(4) sıfır+proto(2) uzunluk(2) */
    uint8_t ps[12];
    put32(ps, local_ip); put32(ps + 4, dst_ip);
    ps[8] = 0; ps[9] = IP_PROTO_UDP; put16(ps + 10, (uint16_t)(8 + len));
    uint32_t s2 = (uint32_t)ones_sum(ps, 12) + ones_sum(uh, 8 + len);
    while (s2 >> 16) s2 = (s2 & 0xFFFF) + (s2 >> 16);
    put16(uh + 6, (uint16_t)~s2);
    int r = ip_sendframe(dst_ip, IP_PROTO_UDP, uh, 8 + len, 0, 0, 1);
    if (r >= 0) {
        udp_socks[s].rip = dst_ip;
        udp_socks[s].rport = dport;
    }
    return r;
}

/* Gelen IPv4 paketini dağıt */
static void ip_handle(const uint8_t* p, int len) {
    if (len < 20) return;
    if ((p[0] >> 4) != 4) return;
    if (ip_cksum(p) != 0) return;
    int ih = (p[0] & 0x0F) * 4;
    if (ih < 20 || len < ih) return;
    uint8_t proto = p[9];
    uint32_t src = get32(p + 12);
    uint32_t dst = get32(p + 16);
    if (dst != local_ip && dst != 0xFFFFFFFFu) return;
    const uint8_t* pl = p + ih;
    int plen = get16(p + 2) - ih;
    if (plen < 0 || plen > len - ih) plen = len - ih;
    if (proto == IP_PROTO_ICMP) icmp_handle(src, pl, plen);
    else if (proto == IP_PROTO_UDP) {
        if (plen >= 8) {
            uint16_t dport = get16(pl + 2);
            for (int i = 0; i < UDP_MAX_SOCK; i++) {
                if (udp_socks[i].used && udp_socks[i].lport == dport) {
                    int n = plen - 8;
                    if (n > (int)sizeof(udp_socks[i].rbuf)) n = sizeof(udp_socks[i].rbuf);
                    for (int k = 0; k < n; k++) udp_socks[i].rbuf[k] = pl[8 + k];
                    udp_socks[i].rlen = n;
                    break;
                }
            }
        }
    }
    else if (proto == IP_PROTO_TCP) {
        tcp_input(p - 14, 14 + ih + plen);
    }
}

void net_handle_packet(const uint8_t* data, int len) {
    if (!data || len < 14) return;
    uint16_t t = get16(data + 12);
    if (t == ETH_TYPE_ARP) arp_handle(data + 14, len - 14);
    else if (t == ETH_TYPE_IP) ip_handle(data + 14, len - 14);
}

void net_send_packet(const uint8_t* data, int len) {
    e1000_send(data, len);
}

void net_init(void) {
    vga_puts("[8B] Network init...\n");
    serial_puts("[8B] Network init...\n");
    memset(arp_cache, 0, sizeof(arp_cache));
    memset(udp_socks, 0, sizeof(udp_socks));
    e1000_init();
    e1000_mac_get(local_mac);
    vga_puts("[8B] Network OK\n");
    serial_puts("[8B] Network OK\n");
}

void net_poll(void) { e1000_poll(); }

/* 21A: gerçek çevrim testi — ARP öğren + ICMP echo yanıtı + UDP TX dolmuş.
 * QEMU slirp gateway (10.0.2.2) ARP ve ping'i yanıtlar. */
int net_selftest(void) {
    if (!e1000_present()) {
        serial_puts("[21A] net selftest atlandı (e1000 yok)\n");
        return -1;
    }
    int t_ok = 1;
    uint32_t gw = 0x0A000202; /* 10.0.2.2 */

    /* 1) MAC geçerli / link */
    if (!e1000_link()) t_ok = 0;
    e1000_poll();

    /* 2) ARP: gateway'i çöz. Birkaç kez iste, DD beklenir. */
    int resolved = 0;
    for (int a = 0; a < 4 && !resolved; a++) {
        arp_request(gw);
        e1000_poll();
        uint32_t t0 = timer_get_ticks();
        while (timer_get_ticks() - t0 < 20) {
            e1000_poll();
            if (arp_lookup(gw)) { resolved = 1; break; }
        }
    }
    if (!resolved) {
        serial_puts("[21A] ARP resolve [FAIL]\n");
        t_ok = 0;
    } else {
        serial_puts("[21A] ARP resolve [PASS]\n");
    }

    /* 3) ICMP echo -> yanıt */
    int echoed = 0;
    if (resolved) {
        net_icmp_reset();
        for (int a = 0; a < 3 && !echoed; a++) {
            net_icmp_send(gw);
            uint32_t t0 = timer_get_ticks();
            while (timer_get_ticks() - t0 < 25) {
                e1000_poll();
                if (net_icmp_hit()) { echoed = 1; break; }
            }
        }
        if (!echoed) {
            serial_puts("[21A] ICMP echo [FAIL]\n");
            t_ok = 0;
        } else {
            serial_puts("[21A] ICMP echo [PASS]\n");
        }
    }

    /* 4) UDP: TX bayrağı — gönderilen çerçevenin e1000'un DD'sini görmesi
     * (real Tx hattının çalıştığının kanıtı). */
    int udp_tx = -1;
    {
        int s = udp_sock_open(44444);
        static const uint8_t upayload[] = { 'c', 'h', 'a', 'r', 'O', 'S' };
        if (s >= 0) {
            udp_tx = udp_sock_send(s, gw, 9, upayload, sizeof(upayload));
        }
    }
    if (udp_tx < 0) {
        serial_puts("[21A] UDP TX [FAIL]\n");
        t_ok = 0;
    } else {
        serial_puts("[21A] UDP TX [PASS]\n");
    }

    serial_puts(t_ok ? "[21A] net real [PASS]\n" : "[21A] net real [FAIL]\n");
    vga_puts(t_ok ? "[21A] net real [PASS]\n" : "[21A] net real [FAIL]\n");
    return t_ok ? 0 : -1;
}