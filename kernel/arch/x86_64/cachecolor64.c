/* 36A: cache coloring — slab kaydirma ofsetleri (L1 aliasing onleme).
 * Her sinif icin donerli ofset; ayni sette ust uste binme dagitilir.
 */
#include "arch/x86_64/longmode.h"

#define CACHECOLOR64_MAX 64

static u64 cachecolor64_counter = 0;

u64 cachecolor64_next(u64 objsz) {
    u64 max, off;
    if (!objsz) return 0;
    max = objsz < CACHECOLOR64_MAX ? objsz : CACHECOLOR64_MAX;
    /* 16 bayt adim: L1 satir dagilimi */
    off = (cachecolor64_counter * 16) % max;
    cachecolor64_counter++;
    return off;
}
