/* 40H: CoW snapshot — ilk-yazim kopyasi + okuma yonlendirme. */
#include "arch/x86_64/longmode.h"

#define SNAP64_MAX 4
#define SNAP64_MAX_CHANGES 64
#define SNAP64_BSIZE 1024

struct snap64_change {
    int used;
    u32 block;
    unsigned char data[SNAP64_BSIZE];
};

struct snap64_set {
    int used;
    struct snap64_change changes[SNAP64_MAX_CHANGES];
};

static struct snap64_set snap64_tab[SNAP64_MAX];
static int (*snap64_backend_rd)(u32 blk, void *buf) = 0;

void snap64_set_backend(int (*rd)(u32 blk, void *buf)) {
    snap64_backend_rd = rd;
}

static struct snap64_set *snap64_get(int id) {
    if (id < 0 || id >= SNAP64_MAX || !snap64_tab[id].used) return 0;
    return &snap64_tab[id];
}

int snap64_create(void) {
    int i, j;
    for (i = 0; i < SNAP64_MAX; i++) {
        if (!snap64_tab[i].used) {
            snap64_tab[i].used = 1;
            for (j = 0; j < SNAP64_MAX_CHANGES; j++)
                snap64_tab[i].changes[j].used = 0;
            return i;
        }
    }
    return -1;
}

/* Ilk yazimda eski icerigi dondur (backend'den okunur). */
int snap64_write(int id, u32 block, const void *old_data) {
    struct snap64_set *s = snap64_get(id);
    const unsigned char *o;
    int i;
    if (!s) return -1;
    for (i = 0; i < SNAP64_MAX_CHANGES; i++)
        if (snap64_tab[id].changes[i].used &&
            snap64_tab[id].changes[i].block == block)
            return 0; /* zaten dondurulmus */
    (void)old_data;
    for (i = 0; i < SNAP64_MAX_CHANGES; i++) {
        if (!snap64_tab[id].changes[i].used) {
            unsigned char *d = snap64_tab[id].changes[i].data;
            int j;
            if (old_data) {
                o = (const unsigned char *)old_data;
                for (j = 0; j < SNAP64_BSIZE; j++) d[j] = o[j];
            } else if (snap64_backend_rd) {
                /* Gecici tampon yok: dogrudan oku */
                if (snap64_backend_rd(block, d) != 0) return -2;
            } else {
                return -3;
            }
            snap64_tab[id].changes[i].used = 1;
            snap64_tab[id].changes[i].block = block;
            return 0;
        }
    }
    return -4; /* degisiklik tablosu dolu */
}

int snap64_read(int id, u32 block, void *out) {
    struct snap64_set *s = snap64_get(id);
    unsigned char *d;
    int i, j;
    if (!s || !out) return -1;
    for (i = 0; i < SNAP64_MAX_CHANGES; i++) {
        if (snap64_tab[id].changes[i].used &&
            snap64_tab[id].changes[i].block == block) {
            d = (unsigned char *)out;
            for (j = 0; j < SNAP64_BSIZE; j++)
                d[j] = snap64_tab[id].changes[i].data[j];
            return 0;
        }
    }
    if (snap64_backend_rd) return snap64_backend_rd(block, out);
    return -2;
}

int snap64_delete(int id) {
    struct snap64_set *s = snap64_get(id);
    int i;
    if (!s) return -1;
    for (i = 0; i < SNAP64_MAX_CHANGES; i++)
        snap64_tab[id].changes[i].used = 0;
    snap64_tab[id].used = 0;
    return 0;
}
