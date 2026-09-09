/* 36E: KASAN-lite — kirmizi bolgeler (redzone) + zehir denetimi.
 * Duzen: [u64 boyut][RZ=32B][kullanici][RZ=32B]; slab64 ustu (<=1976B).
 */
#include "arch/x86_64/longmode.h"

#define KASAN64_RZ 32
#define KASAN64_POISON 0xAB

void *kasan64_alloc(u64 size) {
    unsigned char *raw;
    unsigned char *user;
    u64 total, i;
    if (!size || size + sizeof(u64) + 2 * KASAN64_RZ > 2048) return 0;
    total = sizeof(u64) + 2 * KASAN64_RZ + size;
    raw = (unsigned char *)slab64_alloc(total);
    if (!raw) return 0;
    *(u64 *)raw = size;
    user = raw + sizeof(u64) + KASAN64_RZ;
    for (i = 0; i < KASAN64_RZ; i++) {
        raw[sizeof(u64) + i] = KASAN64_POISON;
        user[size + i] = KASAN64_POISON;
    }
    return user;
}

/* 0=temiz, >0 ilk bozuk ofset+1 (RZ icinde), <0 hata */
int kasan64_check(const void *ptr, u64 size) {
    const unsigned char *raw, *user;
    u64 stored, i;
    if (!ptr) return -1;
    user = (const unsigned char *)ptr;
    raw = user - sizeof(u64) - KASAN64_RZ;
    stored = *(const u64 *)raw;
    if (stored != size) return -2; /* boyut uyusmazligi */
    for (i = 0; i < KASAN64_RZ; i++) {
        if (raw[sizeof(u64) + i] != KASAN64_POISON) return (int)(i + 1);
        if (user[size + i] != KASAN64_POISON)
            return (int)(KASAN64_RZ + i + 1);
    }
    return 0;
}

void kasan64_free(void *ptr, u64 size) {
    unsigned char *raw;
    u64 total;
    if (!ptr) return;
    if (kasan64_check(ptr, size) != 0) return; /* bozuk: birak (debug) */
    raw = (unsigned char *)ptr - sizeof(u64) - KASAN64_RZ;
    total = sizeof(u64) + 2 * KASAN64_RZ + size;
    slab64_free(raw, total);
}
