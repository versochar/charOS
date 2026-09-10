/* 34.3: BAR kodçözüm + boyut yoklama (saf mantık).
 * pci_read_bar'daki gömülü maskeler buraya taşındı; 64-bit dest Cards eklendi.
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "drivers/pcibar.h"
#include "core/verify.h"

/* 34.3: derleme-zamanı kanıtı */
STATIC_ASSERT(sizeof(uint64_t) == 8);

int pcibar_decode(uint32_t lo, uint32_t hi, uint64_t* base,
                  int* is_io, int* is64, int* prefetch) {
    if (REQUIRE(base != 0 && is_io != 0 && is64 != 0 && prefetch != 0,
                0xE501) != 0) return -1;
    if (lo & 1u) { /* IO */
        *is_io = 1;
        *is64 = 0;
        *prefetch = 0;
        *base = (uint64_t)(lo & ~0x3u);
        return 0;
    }
    *is_io = 0;
    *prefetch = (lo & 0x8u) != 0;
    if (((lo >> 1) & 3u) == 2u) { /* 64-bit bellek */
        *is64 = 1;
        *base = (((uint64_t)hi) << 32) | (uint64_t)(lo & ~0xFu);
        return 0;
    }
    /* 32-bit bellek (tip 00; eski tip 01 de buraya düşer) */
    *is64 = 0;
    *base = (uint64_t)(lo & ~0xFu);
    return 0;
}

uint64_t pcibar_size32(uint32_t probe) {
    uint32_t mask;
    if (probe == 0) return 0; /* uygulanmamış BAR */
    mask = probe & ~0xFu;
    return (uint64_t)(~mask) + 1u; /* 32-bit sarmalı bilinçli */
}

uint64_t pcibar_size64(uint32_t lo_probe, uint32_t hi_probe) {
    uint64_t mask;
    if (lo_probe == 0 && hi_probe == 0) return 0;
    mask = (((uint64_t)hi_probe) << 32) | (uint64_t)(lo_probe & ~0xFu);
    return ~mask + 1u; /* 64-bit sarmalı bilinçli */
}

uint64_t pcibar_sizeio(uint32_t probe) {
    uint32_t mask;
    if (probe == 0) return 0;
    mask = probe & ~0x3u & 0xFFFFu;
    if (mask == 0) return 0;
    return (uint64_t)((~mask & 0xFFFFu) + 1u);
}

int pcibar_selftest(void) {
    uint64_t base = 0;
    int io = 0, s64 = 0, pf = 0;
    /* IO BAR */
    if (pcibar_decode(0xC001u, 0, &base, &io, &s64, &pf) != 0) return -1;
    if (!io || s64 || base != 0xC000u) return -2;
    /* 32-bit prefetchable */
    if (pcibar_decode(0xF0000008u, 0, &base, &io, &s64, &pf) != 0) return -3;
    if (io || s64 || !pf || base != 0xF0000000u) return -4;
    /* 64-bit */
    if (pcibar_decode(0x00000014u, 0x00000002u, &base, &io, &s64, &pf) != 0)
        return -5;
    if (io || !s64 || base != 0x200000010ULL) return -6;
    /* boyutlar */
    if (pcibar_size32(0xFFFF0000u) != 0x10000u) return -7;
    if (pcibar_sizeio(0xFFFFFF01u) != 0x100u) return -8;
    if (pcibar_size64(0xFFF00000u, 0xFFFFFFFFu) != 0x100000ULL) return -9;
    if (pcibar_size32(0) != 0 || pcibar_sizeio(0) != 0) return -10;
    if (pcibar_decode(0, 0, 0, &io, &s64, &pf) != -1) return -11;
    return 0;
}
