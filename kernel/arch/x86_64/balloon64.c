/* 35F: balloon — sisme = pmm'den tutulan sayfalar, inme = iade. */
#include "arch/x86_64/longmode.h"

#define BALLOON64_MAX 1024

static u64 balloon64_frames[BALLOON64_MAX];
static u64 balloon64_n = 0;

int balloon64_inflate(u64 pages) {
    u64 i;
    for (i = 0; i < pages; i++) {
        u64 f;
        if (balloon64_n >= BALLOON64_MAX) return -2; /* liste dolu */
        f = pmm64_alloc_frame();
        if (!f) return -1; /* bellek bitti */
        balloon64_frames[balloon64_n++] = f;
    }
    return 0;
}

int balloon64_deflate(u64 pages) {
    u64 i;
    for (i = 0; i < pages && balloon64_n > 0; i++) {
        balloon64_n--;
        pmm64_free_frame(balloon64_frames[balloon64_n]);
        balloon64_frames[balloon64_n] = 0;
    }
    return 0;
}

u64 balloon64_pages(void) { return balloon64_n; }
