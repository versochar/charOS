/* 55I: low latency path — dogrudan tarama + sayfa-cevirme + gecikme. */
#include "arch/x86_64/longmode.h"

#define LOWLATPATH64_MAX 8

struct lowlatpath64_surf {
    int used;
    u32 w, h, format;
    u64 flips;
    u64 last_vblank_us;
};

static struct lowlatpath64_surf lowlatpath64_tab[LOWLATPATH64_MAX];
static u64 lowlatpath64_clock_us = 0;

int lowlatpath64_scanout(u32 w, u32 h, u32 format) {
    int i;
    if (!w || !h || w > 4096 || h > 4096) return -1;
    for (i = 0; i < LOWLATPATH64_MAX; i++) {
        if (!lowlatpath64_tab[i].used) {
            lowlatpath64_tab[i].used = 1;
            lowlatpath64_tab[i].w = w;
            lowlatpath64_tab[i].h = h;
            lowlatpath64_tab[i].format = format;
            lowlatpath64_tab[i].flips = 0;
            lowlatpath64_tab[i].last_vblank_us = 0;
            return i;
        }
    }
    return -2;
}

int lowlatpath64_flip(int id) {
    if (id < 0 || id >= LOWLATPATH64_MAX || !lowlatpath64_tab[id].used)
        return -1;
    /* Skeleton saat: 60Hz vblank (16666us adim) */
    lowlatpath64_clock_us += 16666;
    lowlatpath64_tab[id].flips++;
    lowlatpath64_tab[id].last_vblank_us = lowlatpath64_clock_us;
    return 0;
}

u64 lowlatpath64_vblank(int id) {
    if (id < 0 || id >= LOWLATPATH64_MAX || !lowlatpath64_tab[id].used)
        return 0;
    return lowlatpath64_tab[id].last_vblank_us;
}

u32 lowlatpath64_latency_us(int id) {
    /* Tek karelilk boru hatti gecikmesi (60Hz). */
    (void)id;
    return 16666;
}
