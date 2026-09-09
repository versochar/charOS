/* 44G: AP kipi — istasyon tablosu + isaret cercevesi kurma. */
#include "arch/x86_64/longmode.h"

#define APMODE64_MAX_STA 32

struct apmode64_sta {
    int used;
    unsigned char mac[6];
    int aid;
    int authorized;
};

static struct apmode64_sta apmode64_tab[APMODE64_MAX_STA];
static int apmode64_next_aid = 1;
static int apmode64_n = 0;

static int apmode_mac_eq(const unsigned char *a, const unsigned char *b) {
    int i;
    for (i = 0; i < 6; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

int apmode64_sta_add(const unsigned char mac[6]) {
    int i, k;
    if (!mac) return -1;
    for (i = 0; i < APMODE64_MAX_STA; i++) {
        if (apmode64_tab[i].used && apmode_mac_eq(apmode64_tab[i].mac, mac))
            return 0; /* zaten var */
    }
    for (i = 0; i < APMODE64_MAX_STA; i++) {
        if (!apmode64_tab[i].used) {
            apmode64_tab[i].used = 1;
            for (k = 0; k < 6; k++) apmode64_tab[i].mac[k] = mac[k];
            apmode64_tab[i].aid = apmode64_next_aid++;
            apmode64_tab[i].authorized = 0;
            apmode64_n++;
            return 0;
        }
    }
    return -2;
}

int apmode64_sta_authorize(const unsigned char mac[6]) {
    int i;
    if (!mac) return -1;
    for (i = 0; i < APMODE64_MAX_STA; i++) {
        if (apmode64_tab[i].used && apmode_mac_eq(apmode64_tab[i].mac, mac)) {
            apmode64_tab[i].authorized = 1;
            return 0;
        }
    }
    return -2;
}

int apmode64_sta_remove(const unsigned char mac[6]) {
    int i;
    if (!mac) return -1;
    for (i = 0; i < APMODE64_MAX_STA; i++) {
        if (apmode64_tab[i].used && apmode_mac_eq(apmode64_tab[i].mac, mac)) {
            apmode64_tab[i].used = 0;
            apmode64_n--;
            return 0;
        }
    }
    return -2;
}

/* Minimal isaret: FC(2) + sure(2) + hedef/ff + kaynak(6) + BSSID(6) +
 * sira(2) + SSID elemani + kanal. Donus boy. */
int apmode64_beacon(const char *ssid, int chan, unsigned char *out,
                    int max) {
    int slen = 0, pos;
    static const unsigned char bssid[6] = {0x02, 0, 0, 0, 0, 1};
    int i;
    if (!ssid || chan <= 0 || !out) return -1;
    while (ssid[slen] && slen < 32) slen++;
    if (36 + slen > max) return -2;
    pos = 0;
    out[pos++] = 0x80;
    out[pos++] = 0x00;
    out[pos++] = 0;
    out[pos++] = 0;
    for (i = 0; i < 6; i++) out[pos++] = 0xFF;
    for (i = 0; i < 6; i++) out[pos++] = bssid[i];
    for (i = 0; i < 6; i++) out[pos++] = bssid[i];
    out[pos++] = 0;
    out[pos++] = 0;
    out[pos++] = 0; /* SSID eleman */
    out[pos++] = (unsigned char)slen;
    for (i = 0; i < slen; i++) out[pos++] = (unsigned char)ssid[i];
    out[pos++] = 3; /* DS parametre */
    out[pos++] = 1;
    out[pos++] = (unsigned char)chan;
    return pos;
}

int apmode64_clients(void) { return apmode64_n; }
