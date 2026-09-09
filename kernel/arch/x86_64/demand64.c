/* 32C: demand paging — lazy sifir sayfa.
 * Bolge tablosu + page-fault siniflandirici. err_code bit0: 0=not-present,
 * 1=protection. Yalniz not-present ve kayitli bolge cozulur.
 * Saf C, asm yok (host testi uygun).
 */
#include "arch/x86_64/longmode.h"

#define DEMAND64_MAX 64

struct demand64_region {
    int used;
    u64 base;
    u64 len;
    u64 flags;
};

static struct demand64_region demand64_tab[DEMAND64_MAX];

int demand64_add(u64 base, u64 len, u64 flags) {
    int i;
    if (!len) return -1;
    for (i = 0; i < DEMAND64_MAX; i++) {
        if (!demand64_tab[i].used) {
            demand64_tab[i].used = 1;
            demand64_tab[i].base = base;
            demand64_tab[i].len = len;
            demand64_tab[i].flags = flags;
            return 0;
        }
    }
    return -1;
}

void demand64_remove(u64 base, u64 len) {
    int i;
    (void)len;
    for (i = 0; i < DEMAND64_MAX; i++) {
        if (demand64_tab[i].used && demand64_tab[i].base == base)
            demand64_tab[i].used = 0;
    }
}

int demand64_fault(u64 fault_addr, u64 err_code, u64 *out_frame) {
    int i;
    u64 frame;
    unsigned char *p;
    u64 j;
    if (err_code & 1ULL) return -2; /* protection ihlali, cozulmez */
    for (i = 0; i < DEMAND64_MAX; i++) {
        if (!demand64_tab[i].used) continue;
        if (fault_addr < demand64_tab[i].base) continue;
        if (fault_addr >= demand64_tab[i].base + demand64_tab[i].len) continue;
        frame = pmm64_alloc_frame();
        if (!frame) return -3;
        p = (unsigned char *)pmm64_frame_ptr(frame);
        for (j = 0; j < PMM64_FRAME; j++) p[j] = 0;
        if (out_frame) *out_frame = frame;
        return 0;
    }
    return -1; /* kayitsiz adres */
}
