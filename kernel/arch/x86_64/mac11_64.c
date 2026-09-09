/* 44C: mac80211 iskeleti — arayuz + 4 AC kuyrugu + anahtar tablosu. */
#include "arch/x86_64/longmode.h"

#define MAC11_MAX_IF 4
#define MAC11_QDEPTH 32
#define MAC11_MTU 1600
#define MAC11_MAX_KEYS 4

struct mac11_frame {
    u64 len;
    unsigned char data[MAC11_MTU];
};

struct mac11_iface {
    int used;
    int type;
    unsigned char mac[6];
};

static struct mac11_iface mac11_ifs[MAC11_MAX_IF];
static struct mac11_frame mac11_txq[4][MAC11_QDEPTH];
static struct mac11_frame mac11_rxq[4][MAC11_QDEPTH];
static int mac11_tx_n[4] = {0, 0, 0, 0};
static int mac11_rx_n[4] = {0, 0, 0, 0};
static unsigned char mac11_keys[MAC11_MAX_KEYS][16];
static int mac11_key_set[MAC11_MAX_KEYS] = {0, 0, 0, 0};

int mac11_add_iface(int type, const unsigned char mac[6]) {
    int i, k;
    if (type < 0 || type > 2 || !mac) return -1;
    for (i = 0; i < MAC11_MAX_IF; i++) {
        if (!mac11_ifs[i].used) {
            mac11_ifs[i].used = 1;
            mac11_ifs[i].type = type;
            for (k = 0; k < 6; k++) mac11_ifs[i].mac[k] = mac[k];
            return i;
        }
    }
    return -2;
}

int mac11_tx(int ac, const void *frame, u64 len) {
    const unsigned char *p;
    u64 i;
    if (ac < 0 || ac > 3 || !frame || !len || len > MAC11_MTU) return -1;
    if (mac11_tx_n[ac] >= MAC11_QDEPTH) return -2;
    p = (const unsigned char *)frame;
    mac11_txq[ac][mac11_tx_n[ac]].len = len;
    for (i = 0; i < len; i++)
        mac11_txq[ac][mac11_tx_n[ac]].data[i] = p[i];
    mac11_tx_n[ac]++;
    return 0;
}

int mac11_rx_pending(int ac) {
    if (ac < 0 || ac > 3) return 0;
    return mac11_rx_n[ac];
}

/* Skeleton geri-dongu: tx kuyrugundan rx kuyruguna tasi (suru testi). */
static void mac11_pump(int ac) {
    int i;
    for (i = 0; i < mac11_tx_n[ac] && mac11_rx_n[ac] < MAC11_QDEPTH; i++) {
        u64 j;
        mac11_rxq[ac][mac11_rx_n[ac]].len = mac11_txq[ac][i].len;
        for (j = 0; j < mac11_txq[ac][i].len; j++)
            mac11_rxq[ac][mac11_rx_n[ac]].data[j] =
                mac11_txq[ac][i].data[j];
        mac11_rx_n[ac]++;
    }
    mac11_tx_n[ac] = 0;
}

int mac11_rx(int ac, void *buf, u64 max) {
    unsigned char *d;
    u64 n, i;
    if (ac < 0 || ac > 3 || !buf) return -1;
    mac11_pump(ac);
    if (!mac11_rx_n[ac]) return 0;
    n = mac11_rxq[ac][0].len < max ? mac11_rxq[ac][0].len : max;
    d = (unsigned char *)buf;
    for (i = 0; i < n; i++) d[i] = mac11_rxq[ac][0].data[i];
    for (i = 0; i + 1 < (u64)mac11_rx_n[ac]; i++)
        mac11_rxq[ac][i] = mac11_rxq[ac][i + 1];
    mac11_rx_n[ac]--;
    return (int)n;
}

int mac11_set_key(int idx, const unsigned char key[16]) {
    int i;
    if (idx < 0 || idx >= MAC11_MAX_KEYS || !key) return -1;
    for (i = 0; i < 16; i++) mac11_keys[idx][i] = key[i];
    mac11_key_set[idx] = 1;
    return 0;
}
