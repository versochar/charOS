/* 44E: tarama/dolasim — BSS tablosu + en-guclu secim + esik. */
#include "arch/x86_64/longmode.h"

#define SCAN64_MAX 32
#define SCAN64_ROAM_HYST 10 /* dBm histerezis */
#define SCAN64_ROAM_MIN -75 /* bu altinda dolasim dusun */

struct scan64_bss {
    int used;
    unsigned char bssid[6];
    char ssid[32];
    int chan;
    int rssi;
    int rsn; /* 1=WPA3 */
};

static struct scan64_bss scan64_tab[SCAN64_MAX];
static int scan64_n = 0;

static void scan_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int scan_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int scan64_add(const unsigned char bssid[6], const char *ssid, int chan,
               int rssi, int rsn) {
    int i, k;
    if (!bssid || !ssid || chan <= 0) return -1;
    for (i = 0; i < SCAN64_MAX; i++) {
        int same = 1;
        if (!scan64_tab[i].used) continue;
        for (k = 0; k < 6; k++) {
            if (scan64_tab[i].bssid[k] != bssid[k]) {
                same = 0;
                break;
            }
        }
        if (same) { /* guncelle */
            scan_str_copy(scan64_tab[i].ssid, ssid, 32);
            scan64_tab[i].chan = chan;
            scan64_tab[i].rssi = rssi;
            scan64_tab[i].rsn = rsn ? 1 : 0;
            return 0;
        }
    }
    for (i = 0; i < SCAN64_MAX; i++) {
        if (!scan64_tab[i].used) {
            scan64_tab[i].used = 1;
            for (k = 0; k < 6; k++) scan64_tab[i].bssid[k] = bssid[k];
            scan_str_copy(scan64_tab[i].ssid, ssid, 32);
            scan64_tab[i].chan = chan;
            scan64_tab[i].rssi = rssi;
            scan64_tab[i].rsn = rsn ? 1 : 0;
            scan64_n++;
            return 0;
        }
    }
    return -2;
}

int scan64_best(const char *ssid, unsigned char bssid[6], int *rssi) {
    int i, k, best = -1;
    if (!ssid) return -1;
    for (i = 0; i < SCAN64_MAX; i++) {
        if (!scan64_tab[i].used) continue;
        if (!scan_str_eq(scan64_tab[i].ssid, ssid)) continue;
        if (best < 0 || scan64_tab[i].rssi > scan64_tab[best].rssi)
            best = i;
    }
    if (best < 0) return -2;
    if (bssid)
        for (k = 0; k < 6; k++) bssid[k] = scan64_tab[best].bssid[k];
    if (rssi) *rssi = scan64_tab[best].rssi;
    return 0;
}

int scan64_roam_needed(int cur_rssi, int best_rssi) {
    if (cur_rssi < SCAN64_ROAM_MIN &&
        best_rssi > cur_rssi + SCAN64_ROAM_HYST)
        return 1;
    return 0;
}

int scan64_count(void) { return scan64_n; }
