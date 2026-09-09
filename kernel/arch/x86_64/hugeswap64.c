/* 35H: huge page swap — 2MB sayfayi 512 swap slotuna bol/parcala. */
#include "arch/x86_64/longmode.h"

#define HUGESWAP64_SUBPAGES 512

/* NOT: her alt sayfa ayri eslenir (frame_ptr(huge+i*4K)); identity
 * mapping'de bitisik erisimle ayni sonuc, mapper'li ortamda guvenli. */
int hugeswap64_out(u64 huge_frame, u64 *slots_out) {
    int i;
    if (!huge_frame || !slots_out) return -1;
    for (i = 0; i < HUGESWAP64_SUBPAGES; i++) {
        u64 slot;
        void *sub;
        if (swap64_alloc_slot(&slot) != 0) {
            /* Kismi tahsisi geri al */
            int j;
            for (j = 0; j < i; j++) swap64_free_slot(slots_out[j]);
            return -2;
        }
        slots_out[i] = slot;
        sub = pmm64_frame_ptr(huge_frame + (u64)i * PMM64_FRAME);
        if (swap64_out(slot, sub) != 0) {
            int j;
            for (j = 0; j <= i; j++) swap64_free_slot(slots_out[j]);
            return -3;
        }
    }
    return 0;
}

int hugeswap64_in(u64 huge_frame, const u64 *slots) {
    int i;
    if (!huge_frame || !slots) return -1;
    for (i = 0; i < HUGESWAP64_SUBPAGES; i++) {
        void *sub = pmm64_frame_ptr(huge_frame + (u64)i * PMM64_FRAME);
        if (swap64_in(slots[i], sub) != 0)
            return -2;
        swap64_free_slot(slots[i]);
    }
    return 0;
}
