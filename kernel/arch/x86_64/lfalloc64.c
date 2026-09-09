/* 33I: lock-free kalloc — Treiber stack, CAS (__sync builtin).
 * Havuz caller'dan (statik dizi); objsz >= sizeof(void*) sart.
 * Freestanding uyumlu (libgcc __sync x86_64'te inline lock-prefixed).
 */
#include "arch/x86_64/longmode.h"

static void **lf64_top = 0;
static u64 lf64_objsz = 0;
static u64 lf64_count = 0;

void lf64_init(void *pool, u64 objsz, u64 count) {
    unsigned char *p = (unsigned char *)pool;
    u64 i;
    lf64_top = 0;
    lf64_objsz = 0;
    lf64_count = 0;
    if (!p || objsz < sizeof(void *) || !count) return;
    for (i = 0; i < count; i++) {
        void **slot = (void **)(p + i * objsz);
        void **old;
        do {
            old = lf64_top;
            *slot = old;
        } while (!__sync_bool_compare_and_swap(&lf64_top, old, slot));
    }
    lf64_objsz = objsz;
    lf64_count = count;
}

void *lf64_alloc(void) {
    void **old, *next;
    if (!lf64_top) return 0;
    do {
        old = lf64_top;
        if (!old) return 0;
        next = *old;
    } while (!__sync_bool_compare_and_swap(&lf64_top, old, next));
    return old;
}

void lf64_free(void *obj) {
    void **old;
    if (!obj || !lf64_objsz) return;
    do {
        old = lf64_top;
        *(void **)obj = old;
    } while (!__sync_bool_compare_and_swap(&lf64_top, old, (void **)obj));
}
