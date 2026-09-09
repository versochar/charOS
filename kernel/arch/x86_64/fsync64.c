/* 40C: fsync garantisi — kirli kopya + sira + bariyer. */
#include "arch/x86_64/longmode.h"

#define FSYNC64_MAX 32
#define FSYNC64_BSIZE 1024

struct fsync64_entry {
    int used;
    u32 block;
    unsigned char data[FSYNC64_BSIZE];
};

static struct fsync64_entry fsync64_tab[FSYNC64_MAX];
static int (*fsync64_backend_wr)(u32 blk, const void *buf) = 0;
static u64 fsync64_barrier_seq = 0;

void fsync64_set_backend(int (*wr)(u32 blk, const void *buf)) {
    fsync64_backend_wr = wr;
}

int fsync64_mark_dirty(u32 block, const void *data) {
    const unsigned char *s;
    int i;
    if (!data) return -1;
    for (i = 0; i < FSYNC64_MAX; i++) {
        if (fsync64_tab[i].used && fsync64_tab[i].block == block) {
            s = (const unsigned char *)data;
            for (int j = 0; j < FSYNC64_BSIZE; j++)
                fsync64_tab[i].data[j] = s[j];
            return 0;
        }
    }
    for (i = 0; i < FSYNC64_MAX; i++) {
        if (!fsync64_tab[i].used) {
            fsync64_tab[i].used = 1;
            fsync64_tab[i].block = block;
            s = (const unsigned char *)data;
            for (int j = 0; j < FSYNC64_BSIZE; j++)
                fsync64_tab[i].data[j] = s[j];
            return 0;
        }
    }
    return -2;
}

int fsync64_sync(void) {
    int i, n = 0;
    if (!fsync64_backend_wr) return -1;
    for (i = 0; i < FSYNC64_MAX; i++) {
        if (!fsync64_tab[i].used) continue;
        fsync64_backend_wr(fsync64_tab[i].block, fsync64_tab[i].data);
        fsync64_tab[i].used = 0;
        n++;
    }
    if (n) fsync64_barrier_seq++;
    return n;
}

int fsync64_barrier(void) {
    int n = fsync64_sync();
    if (n < 0) return -1;
    return (int)fsync64_barrier_seq;
}

u64 fsync64_barriers(void) { return fsync64_barrier_seq; }
