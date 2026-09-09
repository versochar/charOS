/* 36F: guard pages — tahsis sonuna zehirli bekci sayfasi + butunluk denetimi.
 * Gercek kernel64'te bekci sayfa unmapped olur; skeleton'da kanarya ile
 * yazma tespiti (ayni API, ayni sozlesme).
 */
#include "arch/x86_64/longmode.h"

#define GUARD64_MAX 32
#define GUARD64_CANARY 0xC0FFEE1234567890ULL

struct guard64_entry {
    int used;
    u64 base;
    int pages;
};

static struct guard64_entry guard64_tab[GUARD64_MAX];

static void guard64_poison(u64 guard_frame) {
    u64 *p = (u64 *)pmm64_frame_ptr(guard_frame);
    u64 i;
    for (i = 0; i < PMM64_FRAME / 8; i++) p[i] = GUARD64_CANARY;
}

u64 guard64_alloc_pages(int n) {
    u64 base = 0;
    int i, j;
    if (n <= 0 || n > 16) return 0;
    /* Bitisik n+1 frame ara (skeleton: tek tek + bitisiklik dogrula) */
    for (j = 0; j < GUARD64_MAX; j++) {
        if (guard64_tab[j].used) continue;
        base = pmm64_alloc_frame();
        if (!base) return 0;
        for (i = 1; i <= n; i++) {
            u64 f = pmm64_alloc_frame();
            if (!f || f != base + (u64)i * PMM64_FRAME) {
                /* Bitisik degil: geri ver, bastan dene (sinirli) */
                if (f) pmm64_free_frame(f);
                pmm64_free_frame(base);
                base = 0;
                break;
            }
        }
        if (base) {
            guard64_tab[j].used = 1;
            guard64_tab[j].base = base;
            guard64_tab[j].pages = n;
            guard64_poison(base + (u64)n * PMM64_FRAME);
            return base;
        }
    }
    return 0;
}

int guard64_check(u64 base) {
    u64 *p;
    u64 i;
    int j;
    for (j = 0; j < GUARD64_MAX; j++) {
        if (guard64_tab[j].used && guard64_tab[j].base == base) {
            p = (u64 *)pmm64_frame_ptr(
                base + (u64)guard64_tab[j].pages * PMM64_FRAME);
            for (i = 0; i < PMM64_FRAME / 8; i++)
                if (p[i] != GUARD64_CANARY) return (int)(i + 1);
            return 0;
        }
    }
    return -1; /* kayitsiz */
}

void guard64_free(u64 base) {
    int j, i;
    for (j = 0; j < GUARD64_MAX; j++) {
        if (guard64_tab[j].used && guard64_tab[j].base == base) {
            for (i = 0; i <= guard64_tab[j].pages; i++)
                pmm64_free_frame(base + (u64)i * PMM64_FRAME);
            guard64_tab[j].used = 0;
            return;
        }
    }
}
