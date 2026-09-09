/* 43B: ARP/NDP onbellegi — TTL'li esleme + olgunlasmamis girdiler. */
#include "arch/x86_64/longmode.h"

#define ARPV4_MAX 32
#define ARPV6_MAX 32
#define ARPNDP64_TTL 300 /* saniye */

struct arp4_entry {
    int used;
    int complete;
    u32 ip;
    unsigned char mac[6];
    u64 expiry;
    int retries;
};

struct arp6_entry {
    int used;
    int complete;
    unsigned char ip[16];
    unsigned char mac[6];
    u64 expiry;
    int retries;
};

static struct arp4_entry arp4_tab[ARPV4_MAX];
static struct arp6_entry arp6_tab[ARPV6_MAX];

int arpndp64_add4(u32 ip, const unsigned char mac[6], u64 now) {
    int i, free = -1;
    for (i = 0; i < ARPV4_MAX; i++) {
        if (arp4_tab[i].used && arp4_tab[i].ip == ip) {
            free = i;
            break;
        }
        if (!arp4_tab[i].used && free < 0) free = i;
    }
    if (free < 0) return -1;
    arp4_tab[free].used = 1;
    arp4_tab[free].ip = ip;
    if (mac) {
        int k;
        for (k = 0; k < 6; k++) arp4_tab[free].mac[k] = mac[k];
        arp4_tab[free].complete = 1;
        arp4_tab[free].retries = 0;
    } else {
        arp4_tab[free].complete = 0; /* cozuluyor */
        arp4_tab[free].retries++;
    }
    arp4_tab[free].expiry = now + ARPNDP64_TTL;
    return 0;
}

int arpndp64_lookup4(u32 ip, unsigned char mac[6], u64 now) {
    int i;
    for (i = 0; i < ARPV4_MAX; i++) {
        int k;
        if (!arp4_tab[i].used || arp4_tab[i].ip != ip) continue;
        if (!arp4_tab[i].complete) return -2; /* bekliyor */
        if (arp4_tab[i].expiry <= now) {
            arp4_tab[i].used = 0;
            return -3; /* suresi dolmus */
        }
        if (mac)
            for (k = 0; k < 6; k++) mac[k] = arp4_tab[i].mac[k];
        return 0;
    }
    return -1; /* yok */
}

static int ip6_eq(const unsigned char *a, const unsigned char *b) {
    int i;
    for (i = 0; i < 16; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

int arpndp64_add6(const unsigned char ip[16], const unsigned char mac[6],
                  u64 now) {
    int i, k, free = -1;
    if (!ip) return -1;
    for (i = 0; i < ARPV6_MAX; i++) {
        if (arp6_tab[i].used && ip6_eq(arp6_tab[i].ip, ip)) {
            free = i;
            break;
        }
        if (!arp6_tab[i].used && free < 0) free = i;
    }
    if (free < 0) return -1;
    arp6_tab[free].used = 1;
    for (k = 0; k < 16; k++) arp6_tab[free].ip[k] = ip[k];
    if (mac) {
        for (k = 0; k < 6; k++) arp6_tab[free].mac[k] = mac[k];
        arp6_tab[free].complete = 1;
        arp6_tab[free].retries = 0;
    } else {
        arp6_tab[free].complete = 0;
        arp6_tab[free].retries++;
    }
    arp6_tab[free].expiry = now + ARPNDP64_TTL;
    return 0;
}

int arpndp64_lookup6(const unsigned char ip[16], unsigned char mac[6],
                     u64 now) {
    int i, k;
    if (!ip) return -1;
    for (i = 0; i < ARPV6_MAX; i++) {
        if (!arp6_tab[i].used || !ip6_eq(arp6_tab[i].ip, ip)) continue;
        if (!arp6_tab[i].complete) return -2;
        if (arp6_tab[i].expiry <= now) {
            arp6_tab[i].used = 0;
            return -3;
        }
        if (mac)
            for (k = 0; k < 6; k++) mac[k] = arp6_tab[i].mac[k];
        return 0;
    }
    return -1;
}

/* Suresi dolmuslari temizler; kalan sayi. */
int arpndp64_tick(u64 now) {
    int i, n = 0;
    for (i = 0; i < ARPV4_MAX; i++) {
        if (!arp4_tab[i].used) continue;
        if (arp4_tab[i].expiry <= now)
            arp4_tab[i].used = 0;
        else
            n++;
    }
    for (i = 0; i < ARPV6_MAX; i++) {
        if (!arp6_tab[i].used) continue;
        if (arp6_tab[i].expiry <= now)
            arp6_tab[i].used = 0;
        else
            n++;
    }
    return n;
}
