/* 44H: mesh iskeleti — hava-suresi metrigi + yol tablosu. */
#include "arch/x86_64/longmode.h"

#define MESH64_MAX 32

struct mesh64_path {
    int used;
    unsigned char dest[6];
    unsigned char hop[6];
    u64 metric;
};

static struct mesh64_path mesh64_tab[MESH64_MAX];

static int mesh_mac_eq(const unsigned char *a, const unsigned char *b) {
    int i;
    for (i = 0; i < 6; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

/* Hava suresi yaklasigi: (bit / hiz) + hata cezasi (us birimi). */
u64 mesh64_metric(u64 rate_mbps, u64 errors) {
    u64 airtime;
    if (!rate_mbps) return ~0ULL;
    airtime = (12000ULL * 1000) / rate_mbps; /* 12Kbit cerceve */
    return airtime + errors * 5000;
}

int mesh64_learn(const unsigned char dest[6], const unsigned char hop[6],
                 u64 metric) {
    int i, k;
    if (!dest || !hop) return -1;
    for (i = 0; i < MESH64_MAX; i++) {
        if (mesh64_tab[i].used && mesh_mac_eq(mesh64_tab[i].dest, dest)) {
            if (metric < mesh64_tab[i].metric) {
                for (k = 0; k < 6; k++) mesh64_tab[i].hop[k] = hop[k];
                mesh64_tab[i].metric = metric;
            }
            return 0;
        }
    }
    for (i = 0; i < MESH64_MAX; i++) {
        if (!mesh64_tab[i].used) {
            int j;
            mesh64_tab[i].used = 1;
            for (j = 0; j < 6; j++) {
                mesh64_tab[i].dest[j] = dest[j];
                mesh64_tab[i].hop[j] = hop[j];
            }
            mesh64_tab[i].metric = metric;
            return 0;
        }
    }
    return -2;
}

int mesh64_route(const unsigned char dest[6], unsigned char hop[6]) {
    int i, k;
    if (!dest) return -1;
    for (i = 0; i < MESH64_MAX; i++) {
        if (mesh64_tab[i].used && mesh_mac_eq(mesh64_tab[i].dest, dest)) {
            if (hop)
                for (k = 0; k < 6; k++) hop[k] = mesh64_tab[i].hop[k];
            return 0;
        }
    }
    return -2; /* yol yok */
}
