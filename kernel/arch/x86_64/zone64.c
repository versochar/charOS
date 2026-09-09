/* 35A: zone yonetimi — LOW/DMA/NORMAL aralik muhasebesi.
 * Varsayim: zone araliklari pmm64_add_region ile pmm'ye eklenmistir.
 * Tahsis pmm64_alloc_range ile, sayaclar burada tutulur.
 */
#include "arch/x86_64/longmode.h"

struct zone64 {
    u64 lo_frame;
    u64 hi_frame;
    u64 used;
};

static struct zone64 zone64_tab[ZONE64_COUNT];
static int zone64_ready = 0;

int zone64_init(void) {
    zone64_tab[ZONE64_LOW].lo_frame = 0;
    zone64_tab[ZONE64_LOW].hi_frame = 256;      /* 1MB / 4K */
    zone64_tab[ZONE64_DMA].lo_frame = 256;
    zone64_tab[ZONE64_DMA].hi_frame = 4096;     /* 16MB / 4K */
    zone64_tab[ZONE64_NORMAL].lo_frame = 4096;
    zone64_tab[ZONE64_NORMAL].hi_frame = PMM64_MAX_MEMORY / PMM64_FRAME;
    for (int i = 0; i < ZONE64_COUNT; i++)
        zone64_tab[i].used = 0;
    zone64_ready = 1;
    return 0;
}

static int zone_of(u64 frame_no) {
    int i;
    for (i = 0; i < ZONE64_COUNT; i++) {
        if (frame_no >= zone64_tab[i].lo_frame &&
            frame_no < zone64_tab[i].hi_frame)
            return i;
    }
    return -1;
}

u64 zone64_alloc(int zone) {
    u64 faddr;
    if (!zone64_ready || zone < 0 || zone >= ZONE64_COUNT) return 0;
    faddr = pmm64_alloc_range(zone64_tab[zone].lo_frame,
                              zone64_tab[zone].hi_frame);
    if (!faddr) return 0;
    zone64_tab[zone].used++;
    return faddr;
}

void zone64_free(u64 frame) {
    int z;
    if (!zone64_ready) return;
    z = zone_of(frame / PMM64_FRAME);
    pmm64_free_frame(frame);
    if (z >= 0 && zone64_tab[z].used > 0) zone64_tab[z].used--;
}

u64 zone64_free_count(int zone) {
    u64 size;
    if (!zone64_ready || zone < 0 || zone >= ZONE64_COUNT) return 0;
    size = zone64_tab[zone].hi_frame - zone64_tab[zone].lo_frame;
    if (zone64_tab[zone].used >= size) return 0;
    return size - zone64_tab[zone].used;
}

u64 zone64_total_count(int zone) {
    if (!zone64_ready || zone < 0 || zone >= ZONE64_COUNT) return 0;
    return zone64_tab[zone].hi_frame - zone64_tab[zone].lo_frame;
}
