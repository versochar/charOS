/* 43J: ipstack64/arpndp64/cubic64/udp64/dhcp64/dns64/tls64/slaac64/
 *      netfilter64 host testi + iperf iskeleti.
 * Calistirma: make test-net64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "arch/x86_64/longmode.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    u32 ip = 0;
    char s[64];
    unsigned char ip6[16];
    unsigned char mac[6];
    struct cubic64 cc;

    CHECK(ipstack64_parse4("192.168.1.10", &ip) == 0 &&
          ip == 0xC0A8010AUL, "43A v4 ayristir");
    CHECK(ipstack64_parse4("999.1.1.1", 0) != 0, "43A aralik red");
    CHECK(ipstack64_parse4("1.2.3", 0) != 0, "43A eksik red");
    CHECK(ipstack64_fmt4(0xC0A8010AUL, s, sizeof(s)) == 0 &&
          !strcmp(s, "192.168.1.10"), "43A v4 bicim");
    CHECK(ipstack64_parse6("2001:db8::1", ip6) == 0 && ip6[0] == 0x20 &&
          ip6[1] == 0x01 && ip6[14] == 0 && ip6[15] == 1, "43A v6 kisaltma");
    CHECK(ipstack64_parse6("fe80::a:b:c:d:e:f:1:2", ip6) != 0,
          "43A v6 asiri red");
    CHECK(ipstack64_fmt6(ip6, s, sizeof(s)) == 0, "43A v6 bicim");
    {
        unsigned char m[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0xFF, 0xFF, 192, 168, 1, 1};
        CHECK(ipstack64_is_v4mapped(m), "43A v4-mapped");
        CHECK(!ipstack64_is_v4mapped(ip6), "43A degil");
    }
    CHECK(ipstack64_subnet4(0xC0A8010AUL, 0xC0A80100UL, 0xFFFFFF00UL),
          "43A alt-ag");
    CHECK(!ipstack64_subnet4(0xC0A8020AUL, 0xC0A80100UL, 0xFFFFFF00UL),
          "43A dis-ag");

    {
        unsigned char m6[6] = {1, 2, 3, 4, 5, 6};
        unsigned char o6[6];
        unsigned char a6[16] = {0xFE, 0x80, 0, 0, 0, 0, 0, 0,
                                0,    0,    0, 0, 0, 0, 0, 1};
        CHECK(arpndp64_add4(0xC0A80101UL, m6, 1000) == 0, "43B arp ekle");
        CHECK(arpndp64_lookup4(0xC0A80101UL, o6, 1001) == 0 &&
              o6[5] == 6, "43B arp bul");
        CHECK(arpndp64_lookup4(0xC0A80102UL, 0, 1001) != 0,
              "43B yok red");
        CHECK(arpndp64_add4(0xC0A80103UL, 0, 1001) == 0, "43B bekleyen");
        CHECK(arpndp64_lookup4(0xC0A80103UL, 0, 1001) == -2,
              "43B cozuluyor");
        CHECK(arpndp64_lookup4(0xC0A80101UL, 0, 2000) != 0,
              "43B ttl doldu");
        CHECK(arpndp64_add6(a6, m6, 1000) == 0, "43B ndp ekle");
        CHECK(arpndp64_lookup6(a6, o6, 1001) == 0, "43B ndp bul");
        CHECK(arpndp64_tick(5000) == 0, "43B temizlik");
    }

    cubic64_init(&cc);
    CHECK(cc.cwnd == 10 * 1024, "43C IW10");
    cubic64_ack(&cc, 1000, 1000000);
    CHECK(cc.cwnd == 11 * 1024, "43C yavas baslangic");
    cc.cwnd = 64 * 1024;
    cc.ssthresh = 32 * 1024;
    cubic64_ack(&cc, 1000, 2000000);  /* epoch kurulur */
    cubic64_ack(&cc, 1000, 12000000); /* t=10sn: disbukey bolge buyur */
    CHECK(cc.cwnd > 64 * 1024, "43C cubic buyume");
    cubic64_loss(&cc);
    CHECK(cc.cwnd < 64 * 1024 && cc.ssthresh == cc.cwnd, "43C kayip");

    {
        int s1 = udp64_socket();
        char rb[64];
        const char *msg = "merhaba udp";
        CHECK(s1 >= 0, "43D soket");
        CHECK(udp64_bind(s1, 0x7F000001UL, 9000) == 0, "43D bagla");
        {
            int s2 = udp64_socket();
            CHECK(udp64_bind(s2, 0, 9000) != 0, "43D port cakisma");
            udp64_close(s2);
        }
        CHECK(udp64_connect(s1, 0x7F000001UL, 9000) == 0, "43D baglan");
        CHECK(udp64_send(s1, msg, strlen(msg)) == (int)strlen(msg),
              "43D gonder");
        CHECK(udp64_recv(s1, rb, sizeof(rb)) == (int)strlen(msg) &&
              !memcmp(rb, msg, strlen(msg)), "43D al");
        CHECK(udp64_recv(s1, rb, sizeof(rb)) == 0, "43D bos");
        CHECK(udp64_close(s1) == 0, "43D kapat");
        CHECK(udp64_send(s1, msg, 1) != 0, "43D kapali red");
    }

    {
        unsigned char out[300], offer[300];
        u32 oip = 0, mask = 0, gw = 0, dns = 0, lease = 0;
        int n;
        CHECK(dhcp64_discover(0x12345678UL, out, sizeof(out)) > 0 &&
              out[242] == 1, "43E discover");
        /* Sentetik OFFER kur */
        memset(offer, 0, sizeof(offer));
        memcpy(offer, out, 244);
        offer[16] = 192;
        offer[17] = 168;
        offer[18] = 1;
        offer[19] = 50;
        n = 240;
        offer[n++] = 53;
        offer[n++] = 1;
        offer[n++] = 2;
        offer[n++] = 1;
        offer[n++] = 4;
        offer[n++] = 255;
        offer[n++] = 255;
        offer[n++] = 255;
        offer[n++] = 0;
        offer[n++] = 51;
        offer[n++] = 4;
        offer[n++] = 0;
        offer[n++] = 0;
        offer[n++] = 0x0E;
        offer[n++] = 0x10;
        offer[n++] = 255;
        CHECK(dhcp64_offer_parse(offer, n, &oip, &mask, 0, 0, &lease) ==
                  0 && oip == 0xC0A80132UL && mask == 0xFFFFFF00UL &&
                  lease == 3600, "43E offer");
        CHECK(dhcp64_request(0x12345678UL, oip, out, sizeof(out)) > 0 &&
              out[242] == 3, "43E request");
        CHECK(dhcp64_bound(oip, lease) == 0, "43E bagla");
        {
            u32 bi = 0, bl = 0;
            CHECK(dhcp64_bound_ip(&bi, &bl) == 0 && bi == oip && bl == 3600,
                  "43E bagli oku");
        }
        CHECK(dhcp64_offer_parse(offer, 10, 0, 0, 0, 0, 0) != 0,
              "43E kisa red");
        (void)gw;
        (void)dns;
    }

    {
        unsigned char q[64], resp[128];
        u32 ips[4];
        int ql, pos;
        CHECK(dns64_query("ornek.test", q, sizeof(q)) > 0, "43F sorgu");
        CHECK(dns64_query("ornek.test", q, 10) != 0, "43F kisa red");
        /* Sentetik yanit: baslik + sorgu yankisi + 1x A */
        memset(resp, 0, sizeof(resp));
        resp[0] = 0x12;
        resp[1] = 0x34;
        resp[2] = 0x81;
        resp[3] = 0x80;
        resp[5] = 1;
        resp[7] = 1;
        {
            /* Soru bolumu yalniz (sorgu basligi HARIC): */
            unsigned char qq[64];
            int full = dns64_query("ornek.test", qq, sizeof(qq));
            ql = full - 12;
            memcpy(resp + 12, qq + 12, (u64)ql);
        }
        pos = 12 + ql;
        resp[pos++] = 0xC0;
        resp[pos++] = 0x0C;
        resp[pos++] = 0;
        resp[pos++] = 1;
        resp[pos++] = 0;
        resp[pos++] = 1;
        resp[pos++] = 0;
        resp[pos++] = 0;
        resp[pos++] = 0x0E;
        resp[pos++] = 0x10;
        resp[pos++] = 0;
        resp[pos++] = 4;
        resp[pos++] = 93;
        resp[pos++] = 184;
        resp[pos++] = 216;
        resp[pos++] = 34;
        CHECK(dns64_parse(resp, pos, ips, 4) == 1 &&
              ips[0] == 0x5DB8D822UL, "43F ayristir");
        CHECK(dns64_parse(resp, 5, ips, 4) != 0, "43F kisa red");
        CHECK(dns64_cache_add("ornek.test", ips[0]) == 0, "43F onbellek");
        {
            u32 ci = 0;
            CHECK(dns64_cached("ORNEK.TEST", &ci) == 0 && ci == ips[0],
                  "43F buyuk-kucuk duyarsiz");
        }
    }

    {
        unsigned char hello[64];
        unsigned char srv[8] = {22, 3, 3, 0, 3, 2, 0, 0};
        CHECK(tls64_client_hello(hello, sizeof(hello)) == 53,
              "43G hello");
        CHECK(tls64_state() == TLS64_HELLO, "43G durum");
        CHECK(tls64_process(srv, sizeof(srv)) == 2, "43G ilerle");
        CHECK(tls64_process(srv, sizeof(srv)) == 3, "43G ilerle2");
        CHECK(tls64_process(srv, sizeof(srv)) == 4, "43G ilerle3");
        CHECK(tls64_process(srv, sizeof(srv)) == TLS64_ESTABLISHED,
              "43G kurulu");
        CHECK(tls64_transcript() != 1469598103934665603ULL,
              "43G transkript");
        CHECK(tls64_process(srv, 2) != 0, "43G kisa red");
    }

    {
        unsigned char mac[6] = {0x02, 0, 0x4C, 0x11, 0x22, 0x33};
        unsigned char pfx[8] = {0xFE, 0x80, 0, 0, 0, 0, 0, 0};
        unsigned char addr[16];
        unsigned char ra[48];
        unsigned char rp[8];
        u32 life = 0;
        CHECK(slaac64_eui64(mac, pfx, addr) == 0 && addr[8] == 0x00 &&
              addr[11] == 0xFF && addr[12] == 0xFE, "43H eui64");
        memset(ra, 0, sizeof(ra));
        ra[0] = 134;
        ra[2] = 0x08;
        ra[3] = 0x00;
        ra[16] = 3;
        ra[17] = 4;
        ra[20] = 0;
        ra[21] = 0;
        ra[22] = 0x0E;
        ra[23] = 0x10;
        memcpy(ra + 32, pfx, 8);
        CHECK(slaac64_ra_parse(ra, sizeof(ra), rp, &life) == 0 &&
              life == 3600 && !memcmp(rp, pfx, 8), "43H ra");
        ra[0] = 135;
        CHECK(slaac64_ra_parse(ra, sizeof(ra), 0, 0) != 0,
              "43H tur red");
        CHECK(slaac64_dad_start(addr) == 0, "43H dad baslat");
        CHECK(slaac64_dad_ok() == 0, "43H dad ok");
        CHECK(slaac64_dad_start(addr) == 0, "43H dad yeniden");
        CHECK(slaac64_dad_conflict() == 0, "43H cakisma");
    }

    {
        struct netfilter64_pkt p = {0xC0A8010AUL, 0xC0A80101UL, 6, 1234,
                                    80};
        CHECK(netfilter64_add(0, 0xC0A80100UL, 0xFFFFFF00UL, 0, 0, 6, 80,
                              NETFILTER64_ACCEPT) == 0, "43I kural");
        CHECK(netfilter64_hook(0, &p) == NETFILTER64_ACCEPT, "43I kabul");
        p.dport = 22;
        CHECK(netfilter64_hook(0, &p) == NETFILTER64_ACCEPT,
              "43I varsayilan kabul");
        CHECK(netfilter64_add(0, 0, 0, 0, 0, 0, 0, NETFILTER64_DROP) == 0,
              "43I yakala-hepsi");
        CHECK(netfilter64_hook(0, &p) == NETFILTER64_DROP, "43I dusur");
        CHECK(netfilter64_hook(1, &p) == NETFILTER64_ACCEPT,
              "43I zincir ayirir");
        CHECK(netfilter64_hook(0, 0) != NETFILTER64_ACCEPT,
              "43I null red");
    }

    /* 43J iperf iskeleti: geri-dongu verimi */
    {
        int s = udp64_socket();
        char chunk[512], back[512];
        clock_t t0;
        int k, n = 0;
        memset(chunk, 0xAB, sizeof(chunk));
        udp64_bind(s, 0, 9999);
        t0 = clock();
        for (k = 0; k < 2000; k++) {
            if (udp64_send(s, chunk, sizeof(chunk)) != (int)sizeof(chunk))
                break;
            if (udp64_recv(s, back, sizeof(back)) != (int)sizeof(chunk))
                break;
            n++;
        }
        {
            double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
            double mb = (double)n * sizeof(chunk) / (1024 * 1024);
            double rate = sec > 0 ? mb / sec : 0;
            printf("43J iperf: %d tur %.3fsn (%.1f MB/sn geri-dongu)\n", n,
                   sec, rate);
            CHECK(n == 2000 && rate > 0, "43J verim");
        }
        udp64_close(s);
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
