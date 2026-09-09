#include <net/tcp.h>
#include <net/net.h>
#include <net/e1000.h>
#include <drivers/timer.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 10A: eski minimal API (davranış korunur) */
static int tcp_connected = 0;
static int tcp_client_connected = 0;

void tcp_server_start(void) {
    vga_puts("[10A] TCP server start (port 80)\n");
    serial_puts("[10A] TCP server start\n");
    tcp_connected = 1;
    tcp_client_connected = 0;
}

void tcp_client_connect(const char* ip_str, int port) {
    (void)ip_str; (void)port;
    vga_puts("[11A] TCP client connect to port 80\n");
    serial_puts("[11A] TCP client connect\n");
    tcp_client_connected = 1;
}

void tcp_send(const char* data, int len) {
    (void)data; (void)len;
    // Minimal: gerçek TCP segment oluşturma ileride genişletilecek
}

int tcp_recv(char* buf, int len) {
    if (!tcp_connected) return -1;
    const char* resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\ncharOS TCP OK v1.0\n";
    int to_copy = strlen(resp);
    if (to_copy > len) to_copy = len;
    memcpy(buf, resp, to_copy);
    return to_copy;
}

/* ===== 14I: TCP v2 çekirdeği (durum + yeniden iletim + pencere) ===== */

#define TCP_MAXPCB 4
#define TCP_RBUF 1024
#define TCP_TXLOG 16
#define TCP_HDRLEN 20
#define TCP_IPLEN 20
#define TCP_ETHLEN 14
#define TCP_MAXRETX 5

struct tcp_pcb {
    int used;
    int state;
    uint8_t lip[4];
    uint8_t rip[4];
    uint16_t lport, rport;
    uint32_t snd_nxt, snd_una;
    uint32_t rcv_nxt;
    uint16_t rcv_wnd;
    uint16_t peer_wnd;
    /* yeniden iletim yuvası (tek olağan segment) */
    uint8_t rex_buf[TCP_MSS];
    uint16_t rex_len;
    uint32_t rex_seq;
    uint8_t rex_flags;
    uint32_t rex_deadline;
    uint8_t rex_count;
    int rex_active;
    int dupacks;
    uint8_t rbuf[TCP_RBUF];
    int rlen;
};

struct txe {
    uint32_t seq, ack;
    uint8_t flags;
    uint16_t len;
    uint8_t data[64];
};

static struct tcp_pcb pcbs[TCP_MAXPCB];
static struct txe txlog[TCP_TXLOG];
static int tx_n = 0;
static int tcp_on = 0;
static uint16_t next_port = 40000;
static uint32_t tcp_rto = 30;      /* 100Hz tick */
static uint16_t tcp_def_wnd = 1024;

static const uint8_t local_ip[4] = {10, 0, 2, 15};
static const uint8_t local_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static const uint8_t remote_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x57};

static void put16(uint8_t* p, uint16_t v) { p[0] = v >> 8; p[1] = v & 0xFF; }
static void put32(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF; p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF; p[3] = v & 0xFF;
}
static uint16_t get16(const uint8_t* p) { return ((uint16_t)p[0] << 8) | p[1]; }
static uint32_t get32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

/* 16-bit ones-complement toplam */
static uint16_t tcp_cksum(const uint8_t* src, const uint8_t* dst,
                          const uint8_t* seg, int seglen) {
    uint32_t sum = 0;
    sum += ((uint16_t)src[0] << 8) | src[1];
    sum += ((uint16_t)src[2] << 8) | src[3];
    sum += ((uint16_t)dst[0] << 8) | dst[1];
    sum += ((uint16_t)dst[2] << 8) | dst[3];
    sum += 6; /* TCP */
    sum += (uint16_t)seglen;
    for (int i = 0; i + 1 < seglen; i += 2)
        sum += ((uint16_t)seg[i] << 8) | seg[i + 1];
    if (seglen & 1) sum += (uint16_t)seg[seglen - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static uint16_t ip_cksum(const uint8_t* h) {
    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2)
        sum += ((uint16_t)h[i] << 8) | h[i + 1];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static struct tcp_pcb* pcb_get(int id) {
    if (id < 0 || id >= TCP_MAXPCB || !pcbs[id].used) return 0;
    return &pcbs[id];
}

int tcp_init(void) {
    memset(pcbs, 0, sizeof(pcbs));
    memset(txlog, 0, sizeof(txlog));
    tx_n = 0;
    tcp_on = 1;
    return 0;
}

static void tx_log(uint32_t seq, uint32_t ack, uint8_t flags,
                   const uint8_t* data, int len) {
    if (tx_n >= TCP_TXLOG) return;
    txlog[tx_n].seq = seq;
    txlog[tx_n].ack = ack;
    txlog[tx_n].flags = flags;
    txlog[tx_n].len = (uint16_t)(len > 64 ? 64 : len);
    for (int i = 0; i < txlog[tx_n].len; i++) txlog[tx_n].data[i] = data[i];
    tx_n++;
}

int tcp_tx_count(void) { return tx_n; }

int tcp_tx_get(int i, uint32_t* seq, uint32_t* ack, uint8_t* flags, int* len) {
    if (i < 0 || i >= tx_n) return -1;
    if (seq) *seq = txlog[i].seq;
    if (ack) *ack = txlog[i].ack;
    if (flags) *flags = txlog[i].flags;
    if (len) *len = txlog[i].len;
    return 0;
}

int tcp_tx_data(int i, char* out, int max) {
    if (i < 0 || i >= tx_n || !out || max <= 0) return -1;
    int n = txlog[i].len;
    if (n > max) n = max;
    for (int k = 0; k < n; k++) out[k] = (char)txlog[i].data[k];
    return n;
}

/* Çerçeve kur: eth + ipv4 + tcp + veri. Dönüş: toplam bayt / -1 sığmadı. */
static int frame_build(uint8_t* out, int max,
                       const uint8_t* src_ip, const uint8_t* dst_ip,
                       uint16_t sport, uint16_t dport,
                       uint32_t seq, uint32_t ack, uint8_t flags,
                       uint16_t wnd, const uint8_t* data, int dlen) {
    int total = TCP_ETHLEN + TCP_IPLEN + TCP_HDRLEN + dlen;
    if (total > max) return -1;
    for (int i = 0; i < 6; i++) { out[i] = remote_mac[i]; out[6 + i] = local_mac[i]; }
    /* Eş yönü: her iki yönde de aynı MAC'ler (loopback sadeliği) */
    put16(out + 12, 0x0800);
    uint8_t* ip = out + TCP_ETHLEN;
    ip[0] = 0x45; ip[1] = 0;
    put16(ip + 2, (uint16_t)(TCP_IPLEN + TCP_HDRLEN + dlen));
    put16(ip + 4, 0x1234);
    put16(ip + 6, 0x4000);
    ip[8] = 64; ip[9] = 6;
    put16(ip + 10, 0);
    for (int i = 0; i < 4; i++) { ip[12 + i] = src_ip[i]; ip[16 + i] = dst_ip[i]; }
    put16(ip + 10, ip_cksum(ip));
    uint8_t* th = ip + TCP_IPLEN;
    put16(th, sport); put16(th + 2, dport);
    put32(th + 4, seq); put32(th + 8, ack);
    th[12] = 0x50; th[13] = flags;
    put16(th + 14, wnd);
    put16(th + 16, 0);
    put16(th + 18, 0);
    for (int i = 0; i < dlen; i++) th[TCP_HDRLEN + i] = data[i];
    put16(th + 16, tcp_cksum(src_ip, dst_ip, th, TCP_HDRLEN + dlen));
    return total;
}

/* Gelen çerçeve çözümle + doğrula. 0 ok (çıktılar doldurulur). */
static int frame_parse(const uint8_t* f, int len,
                       uint8_t* src_ip, uint8_t* dst_ip,
                       uint16_t* sport, uint16_t* dport,
                       uint32_t* seq, uint32_t* ack, uint8_t* flags,
                       uint16_t* wnd, const uint8_t** data, int* dlen) {
    if (!f || len < TCP_ETHLEN + TCP_IPLEN + TCP_HDRLEN) return -1;
    if (f[12] != 0x08 || f[13] != 0x00) return -1;
    const uint8_t* ip = f + TCP_ETHLEN;
    if ((ip[0] >> 4) != 4 || (ip[0] & 0x0F) != 5) return -1;
    if (ip[9] != 6) return -1;
    if (ip_cksum(ip) != 0) return -1;
    int iplen = ((ip[2] << 8) | ip[3]);
    if (iplen < TCP_IPLEN + TCP_HDRLEN || TCP_ETHLEN + iplen > len) return -1;
    const uint8_t* th = ip + TCP_IPLEN;
    if ((th[12] >> 4) != 5) return -1;
    int tlen = iplen - TCP_IPLEN;
    if (tcp_cksum(ip + 12, ip + 16, th, tlen) != 0) return -1;
    for (int i = 0; i < 4; i++) { src_ip[i] = ip[12 + i]; dst_ip[i] = ip[16 + i]; }
    *sport = get16(th); *dport = get16(th + 2);
    *seq = get32(th + 4); *ack = get32(th + 8);
    *flags = th[13]; *wnd = get16(th + 14);
    *data = th + TCP_HDRLEN;
    *dlen = tlen - TCP_HDRLEN;
    return 0;
}

static struct tcp_pcb* pcb_lookup(uint16_t dport, uint16_t sport,
                                  const uint8_t* src_ip) {
    (void)src_ip;
    for (int i = 0; i < TCP_MAXPCB; i++) {
        if (!pcbs[i].used) continue;
        if (pcbs[i].lport == dport && pcbs[i].rport == sport) return &pcbs[i];
    }
    return 0;
}

/* seq açık parametre: yeniden iletimler rex_seq ile çıkar (snd_nxt değil) */
static void tcp_send_seg_at(struct tcp_pcb* p, uint32_t seq, uint8_t flags,
                            const uint8_t* data, int dlen, int arm) {
    uint8_t frame[TCP_ETHLEN + TCP_IPLEN + TCP_HDRLEN + TCP_MSS];
    int n = frame_build(frame, sizeof(frame), p->lip, p->rip,
                        p->lport, p->rport, seq,
                        (flags & TCP_ACK) ? p->rcv_nxt : 0,
                        flags, p->rcv_wnd, data ? data : (const uint8_t*)"",
                        data ? dlen : 0);
    if (n < 0) return;
    tx_log(seq, (flags & TCP_ACK) ? p->rcv_nxt : 0, flags,
           data, data ? dlen : 0);
    net_send_packet(frame, n);
    if (arm && (dlen > 0 || (flags & (TCP_SYN | TCP_FIN)))) {
        /* Yeniden iletim yuvası kur */
        int cpy = dlen > TCP_MSS ? TCP_MSS : dlen;
        for (int i = 0; i < cpy; i++) p->rex_buf[i] = data[i];
        p->rex_len = (uint16_t)cpy;
        p->rex_seq = seq;
        p->rex_flags = flags;
        p->rex_deadline = timer_get_ticks() + tcp_rto;
        p->rex_count = 0;
        p->rex_active = 1;
        p->dupacks = 0;
    }
}

static void tcp_send_seg(struct tcp_pcb* p, uint8_t flags,
                         const uint8_t* data, int dlen) {
    tcp_send_seg_at(p, p->snd_nxt, flags, data, dlen, 1);
}

/* Yeniden iletim: tel+log rex_seq ile, snd_nxt'e dokunmadan */
static void tcp_resend(struct tcp_pcb* p) {
    tcp_send_seg_at(p, p->rex_seq, p->rex_flags, p->rex_buf, p->rex_len, 1);
    p->rex_deadline = timer_get_ticks() + tcp_rto;
    p->rex_count++;
    p->dupacks = 0;
}

int tcp_connect(uint32_t remote_ip, uint16_t remote_port) {
    if (!tcp_on) tcp_init();
    int id = -1;
    for (int i = 0; i < TCP_MAXPCB; i++)
        if (!pcbs[i].used) { id = i; break; }
    if (id < 0) return -1;
    struct tcp_pcb* p = &pcbs[id];
    memset(p, 0, sizeof(*p));
    p->used = 1;
    p->state = TCP_STATE_SYN_SENT;
    for (int i = 0; i < 4; i++) {
        p->lip[i] = local_ip[i];
        p->rip[i] = (uint8_t)((remote_ip >> (24 - 8 * i)) & 0xFF);
    }
    p->lport = next_port++;
    if (next_port < 40000) next_port = 40000;
    p->rport = remote_port;
    p->snd_nxt = timer_get_ticks() + (uint32_t)id * 0x10000 + 1000;
    p->snd_una = p->snd_nxt;
    p->rcv_nxt = 0;
    p->rcv_wnd = tcp_def_wnd;
    p->peer_wnd = 65535;
    uint32_t iss = p->snd_nxt;
    tcp_send_seg(p, TCP_SYN, 0, 0);
    p->snd_nxt = iss + 1; /* SYN 1 sıra tüketir */
    return id;
}

int tcp_send_data(int id, const char* data, int len) {
    struct tcp_pcb* p = pcb_get(id);
    if (!p || p->state != TCP_STATE_ESTABLISHED) return -1;
    if (!data || len <= 0) return -1;
    if (len > TCP_MSS) len = TCP_MSS;
    if ((uint32_t)len > p->peer_wnd) len = p->peer_wnd;
    if (len <= 0) return 0;
    tcp_send_seg(p, TCP_ACK | TCP_PSH, (const uint8_t*)data, len);
    p->snd_nxt += (uint32_t)len;
    return len;
}

int tcp_input(const uint8_t* frame, int len) {
    uint8_t src[4], dst[4];
    uint16_t sport, dport, wnd;
    uint32_t seq, ack;
    uint8_t flags;
    const uint8_t* data;
    int dlen;
    if (frame_parse(frame, len, src, dst, &sport, &dport, &seq, &ack,
                    &flags, &wnd, &data, &dlen) != 0)
        return -1;
    /* Bize mi? (yerel IP + bilinen port) */
    int local = 1;
    for (int i = 0; i < 4; i++)
        if (dst[i] != local_ip[i]) local = 0;
    if (!local) return -1;
    struct tcp_pcb* p = pcb_lookup(dport, sport, src);
    if (!p) return -1;
    p->peer_wnd = wnd;
    if (flags & TCP_RST) {
        p->state = TCP_STATE_CLOSED;
        p->rex_active = 0;
        return 0;
    }
    if (p->state == TCP_STATE_SYN_SENT) {
        if ((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK) &&
            ack == p->snd_nxt) {
            p->rcv_nxt = seq + 1;
            p->snd_una = ack;
            p->state = TCP_STATE_ESTABLISHED;
            p->rex_active = 0;
            p->dupacks = 0;
            tcp_send_seg(p, TCP_ACK, 0, 0);
            return 0;
        }
        return -1;
    }
    if (p->state != TCP_STATE_ESTABLISHED &&
        p->state != TCP_STATE_CLOSE_WAIT)
        return -1;
    /* Veri: sıra içi + pencere içi ise kabul et */
    if (dlen > 0) {
        if (seq == p->rcv_nxt && dlen <= (int)p->rcv_wnd) {
            int room = TCP_RBUF - p->rlen;
            int cpy = dlen < room ? dlen : room;
            for (int i = 0; i < cpy; i++) p->rbuf[p->rlen++] = data[i];
            p->rcv_nxt += (uint32_t)cpy;
        }
        tcp_send_seg(p, TCP_ACK, 0, 0);
    }
    /* FIN: kapatma beklemesi */
    if ((flags & TCP_FIN) && seq == p->rcv_nxt) {
        p->rcv_nxt++;
        p->state = TCP_STATE_CLOSE_WAIT;
        tcp_send_seg(p, TCP_ACK, 0, 0);
    }
    /* ACK işleme */
    if (flags & TCP_ACK) {
        if (ack > p->snd_una && ack <= p->snd_nxt) {
            p->snd_una = ack;
            p->dupacks = 0;
            if (p->snd_una == p->snd_nxt) p->rex_active = 0;
        } else if (ack == p->snd_una && p->rex_active && dlen == 0) {
            p->dupacks++;
            if (p->dupacks >= 3) {
                /* Hızlı yeniden iletim */
                tcp_resend(p);
            }
        }
    }
    return 0;
}

void tcp_tick(void) {
    if (!tcp_on) return;
    uint32_t now = timer_get_ticks();
    for (int i = 0; i < TCP_MAXPCB; i++) {
        struct tcp_pcb* p = &pcbs[i];
        if (!p->used || !p->rex_active) continue;
        if ((int32_t)(now - p->rex_deadline) < 0) continue;
        if (p->rex_count >= TCP_MAXRETX) {
            p->state = TCP_STATE_CLOSED;
            p->rex_active = 0;
            continue;
        }
        tcp_resend(p);
        p->rex_deadline = now + tcp_rto;
    }
}

/* Test kancaları */
void tcp_set_rto(uint32_t ticks) { tcp_rto = ticks ? ticks : 1; }
void tcp_set_rwnd(int id, uint16_t wnd) {
    struct tcp_pcb* p = pcb_get(id);
    if (p) p->rcv_wnd = wnd;
}

int tcp_state(int id) {
    struct tcp_pcb* p = pcb_get(id);
    return p ? p->state : -1;
}
uint32_t tcp_snd_nxt(int id) {
    struct tcp_pcb* p = pcb_get(id);
    return p ? p->snd_nxt : 0;
}
uint32_t tcp_snd_una(int id) {
    struct tcp_pcb* p = pcb_get(id);
    return p ? p->snd_una : 0;
}
uint32_t tcp_rcv_nxt(int id) {
    struct tcp_pcb* p = pcb_get(id);
    return p ? p->rcv_nxt : 0;
}
int tcp_rlen(int id) {
    struct tcp_pcb* p = pcb_get(id);
    return p ? p->rlen : -1;
}
int tcp_rbuf_get(int id, char* out, int max) {
    struct tcp_pcb* p = pcb_get(id);
    if (!p || !out || max <= 0) return -1;
    int n = p->rlen < max ? p->rlen : max;
    for (int i = 0; i < n; i++) out[i] = (char)p->rbuf[i];
    return n;
}

int tcp_craft(uint8_t* out, int max, uint8_t flags, uint32_t seq,
              uint32_t ack, const char* data, int len) {
    struct tcp_pcb* p = 0;
    for (int i = 0; i < TCP_MAXPCB; i++)
        if (pcbs[i].used) { p = &pcbs[i]; break; }
    if (!p) return -1;
    if (len < 0) len = 0;
    return frame_build(out, max, p->rip, p->lip, p->rport, p->lport,
                       seq, ack, flags, 1024,
                       (const uint8_t*)data, data ? len : 0);
}

int tcp_inject_raw(const uint8_t* frame, int len) {
    return tcp_input(frame, len);
}

int tcp_inject(int id, uint8_t flags, uint32_t seq, uint32_t ack,
               const char* data, int len) {
    struct tcp_pcb* p = pcb_get(id);
    if (!p) return -1;
    uint8_t frame[TCP_ETHLEN + TCP_IPLEN + TCP_HDRLEN + TCP_MSS];
    int n = frame_build(frame, sizeof(frame), p->rip, p->lip,
                        p->rport, p->lport, seq, ack, flags, 1024,
                        (const uint8_t*)data, data ? len : 0);
    if (n < 0) return -1;
    return tcp_input(frame, n);
}

int tcp_selftest(void) {
    int ok = 1;
    tcp_init();
    tcp_set_rto(30);
    /* 1) El sıkışma */
    int tx0 = tcp_tx_count();
    int c = tcp_connect(0x0A000202u, 80);
    if (c < 0) ok = 0;
    uint32_t iss = 0;
    if (ok) {
        uint32_t sq = 0, ak = 0;
        uint8_t fl = 0;
        int ln = -1;
        tcp_tx_get(tx_n - 1, &sq, &ak, &fl, &ln);
        if (!(fl & TCP_SYN) || ln != 0) ok = 0;
        iss = sq;
        if (tcp_state(c) != TCP_STATE_SYN_SENT) ok = 0;
        /* SYN-ACK */
        if (tcp_inject(c, TCP_SYN | TCP_ACK, 5000, iss + 1, 0, 0) != 0) ok = 0;
        if (tcp_state(c) != TCP_STATE_ESTABLISHED) ok = 0;
        if (tcp_snd_una(c) != iss + 1) ok = 0;
        if (tcp_rcv_nxt(c) != 5001) ok = 0;
        if (tcp_tx_count() != tx0 + 2) ok = 0; /* SYN + ACK */
    }
    serial_puts("[14I] dbg hs="); serial_puthex((uint32_t)ok); serial_puts("\n");
    /* 2) Veri + ACK */
    if (ok) {
        int w = tcp_send_data(c, "HELLO", 5);
        if (w != 5) ok = 0;
        uint32_t nxt = tcp_snd_nxt(c);
        if (tcp_inject(c, TCP_ACK, 5001, nxt, 0, 0) != 0) ok = 0;
        if (tcp_snd_una(c) != nxt) ok = 0;
    }
    /* 3) Hızlı yeniden iletim: 3 yinelenen ACK */
    if (ok) {
        int before = tcp_tx_count();
        uint32_t nxt = tcp_snd_nxt(c);
        if (tcp_send_data(c, "WORLD", 5) != 5) ok = 0;
        for (int k = 0; k < 3; k++)
            tcp_inject(c, TCP_ACK, 5001, nxt, 0, 0);
        serial_puts("[14I] dbg tx="); serial_puthex((uint32_t)tcp_tx_count());
        serial_puts(" want="); serial_puthex((uint32_t)(before + 2));
        serial_puts(" una="); serial_puthex(tcp_snd_una(c));
        serial_puts(" nxt="); serial_puthex(tcp_snd_nxt(c));
        serial_puts("\n");
        if (tcp_tx_count() != before + 2) ok = 0; /* veri + hızlı-tx */
        {
            uint32_t sq = 0; int ln = -1; uint8_t fl = 0;
            tcp_tx_get(tcp_tx_count() - 1, &sq, 0, &fl, &ln);
            char db[8];
            tcp_tx_data(tcp_tx_count() - 1, db, sizeof(db));
            if (sq != nxt || ln != 5) ok = 0;
            if (db[0] != 'W' || db[4] != 'D') ok = 0;
            (void)fl;
        }
        /* Temizle: her şeyi onayla */
        if (tcp_inject(c, TCP_ACK, 5001, tcp_snd_nxt(c), 0, 0) != 0) ok = 0;
    }
    serial_puts("[14I] dbg fast="); serial_puthex((uint32_t)ok); serial_puts("\n");
    /* 4) RTO zaman aşımı: küçük RTO + bekle + tick */
    if (ok) {
        tcp_set_rto(3);
        int before = tcp_tx_count();
        if (tcp_send_data(c, "ZZZ", 3) != 3) ok = 0;
        /* ACK yok: süre dolumu beklenir */
        {
            extern uint32_t timer_get_ticks(void);
            uint32_t t0 = timer_get_ticks();
            while (timer_get_ticks() - t0 < 10) { }
        }
        tcp_tick();
        tcp_set_rto(30);
        if (tcp_tx_count() != before + 2) ok = 0; /* veri + rto-tx */
        if (tcp_inject(c, TCP_ACK, 5001, tcp_snd_nxt(c), 0, 0) != 0) ok = 0;
    }
    serial_puts("[14I] dbg rto="); serial_puthex((uint32_t)ok); serial_puts("\n");
    /* 5) Pencere: daralt, taşan veri düşer */
    if (ok) {
        tcp_set_rwnd(c, 8);
        int rl = tcp_rlen(c);
        if (tcp_inject(c, TCP_ACK | TCP_PSH, 5001, tcp_snd_una(c),
                       "0123456789ABCDEF", 16) != 0) ok = 0;
        if (tcp_rlen(c) != rl) ok = 0; /* düşmeli */
        tcp_set_rwnd(c, 1024);
        if (tcp_inject(c, TCP_ACK | TCP_PSH, 5001, tcp_snd_una(c),
                       "hi", 2) != 0) ok = 0;
        if (tcp_rlen(c) != rl + 2) ok = 0;
    }
    serial_puts("[14I] dbg win="); serial_puthex((uint32_t)ok); serial_puts("\n");
    /* 6) Bozuk checksum düşer */
    if (ok) {
        uint8_t fr[128];
        int n = tcp_craft(fr, sizeof(fr), TCP_ACK, 5001, tcp_snd_una(c), 0, 0);
        if (n > 0) {
            fr[n - 1] ^= 0xFF;
            int txb = tcp_tx_count();
            int rl = tcp_rlen(c);
            tcp_inject_raw(fr, n);
            if (tcp_tx_count() != txb || tcp_rlen(c) != rl) ok = 0;
        } else ok = 0;
    }
    if (ok) {
        serial_puts("[14I] tcp rto/window/cksum [PASS]\n");
        vga_puts("[14I] tcp rto/window/cksum [PASS]\n");
        return 0;
    }
    serial_puts("[14I] tcp [FAIL]\n");
    vga_puts("[14I] tcp [FAIL]\n");
    return -1;
}
