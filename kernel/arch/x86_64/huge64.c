/* 32G: huge pages — 2MB identity harita kurucu + 1GB PDP girdisi.
 * Tablolari caller verir (statik havuz); saf C, asm yok (host testi uygun).
 * Duzen: pml4[0] -> pdp; pdp[i] -> pd_pool[i]; pd[j] = 2MB PS sayfasi.
 */
#include "arch/x86_64/longmode.h"

#define HUGE64_2MB (2ULL*1024*1024)
#define HUGE64_1GB (1024ULL*1024*1024)

int paging64_build_identity(u64 *pml4, u64 *pdp, u64 *pd_pool,
                            int pd_count, u64 len, u64 flags) {
    u64 need_gb, off;
    u64 i;
    int g;
    u64 j;
    if (!pml4 || !pdp || !pd_pool || pd_count <= 0) return -1;
    len = (len + HUGE64_2MB - 1) & ~(HUGE64_2MB - 1);
    need_gb = (len + HUGE64_1GB - 1) / HUGE64_1GB;
    if ((u64)pd_count < need_gb) return -2;
    for (i = 0; i < 512; i++) pml4[i] = 0;
    for (i = 0; i < 512; i++) pdp[i] = 0;
    pml4[0] = ((u64)pdp & ~0xFFFULL) | (flags & ~PAGE_PS64) | PAGE_PRESENT64;
    off = 0;
    for (g = 0; g < (int)need_gb; g++) {
        u64 *pd = pd_pool + (u64)g * 512;
        for (j = 0; j < 512; j++) pd[j] = 0;
        pdp[g] = ((u64)pd & ~0xFFFULL) | (flags & ~PAGE_PS64) | PAGE_PRESENT64;
        for (j = 0; j < 512 && off < len; j++, off += HUGE64_2MB) {
            pd[j] = (off & ~(HUGE64_2MB - 1)) |
                    (flags & ~PAGE_PS64) | PAGE_PS64 | PAGE_PRESENT64;
        }
    }
    return 0;
}

void paging64_map_1gb_at(u64 *pdp, int idx, u64 phys, u64 flags) {
    if (!pdp || idx < 0 || idx >= 512) return;
    pdp[idx] = (phys & ~(HUGE64_1GB - 1)) |
               (flags & ~PAGE_PS64) | PAGE_PS64 | PAGE_PRESENT64;
}
