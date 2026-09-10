/* 34.5: GERÇEK kernel/drivers/pcibar.c testi (aynı dosya derlenir).
 * Calistirma: make test-pcibar34
 */
#include <stdio.h>
#include "drivers/pcibar.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    uint64_t base = 0;
    int io = 0, s64 = 0, pf = 0;

    /* 34.3: IO BAR */
    CHECK(pcibar_decode(0xC001u, 0, &base, &io, &s64, &pf) == 0 &&
          io && !s64 && base == 0xC000u, "34.3 io");

    /* 34.3: 64-bit prefetchable */
    CHECK(pcibar_decode(0x0000001Cu, 0x00000003u, &base, &io, &s64, &pf) == 0 &&
          !io && s64 && pf && base == 0x300000010u, "34.3 mem64");

    /* 34.3: 32-bit non-prefetch */
    CHECK(pcibar_decode(0xF0000000u, 0xDEADu, &base, &io, &s64, &pf) == 0 &&
          !io && !s64 && !pf && base == 0xF0000000u, "34.3 mem32");

    /* 34.3: boyut yoklamaları */
    CHECK(pcibar_size32(0xFFFF0000u) == 0x10000u, "34.3 boyut32");
    CHECK(pcibar_sizeio(0xFFFFFF01u) == 0x100u, "34.3 boyutio");
    CHECK(pcibar_size64(0xFFF00000u, 0xFFFFFFFFu) == 0x100000ULL, "34.3 boyut64");
    CHECK(pcibar_size32(0) == 0 && pcibar_sizeio(0) == 0 &&
          pcibar_size64(0, 0) == 0, "34.3 bos");

    /* 34.3: parametre hataları */
    CHECK(pcibar_decode(0, 0, 0, &io, &s64, &pf) == -1, "34.3 null");

    /* 34.3: öztest */
    CHECK(pcibar_selftest() == 0, "34.3 selftest");

    /* 34.9: deterministik tekrar */
    {
        uint64_t b1 = 0, b2 = 0;
        int i1 = 0, i2 = 0, g1 = 0, g2 = 0, f1 = 0, f2 = 0;
        int r1 = pcibar_decode(0xC001u, 0, &b1, &i1, &g1, &f1);
        int r2 = pcibar_decode(0xC001u, 0, &b2, &i2, &g2, &f2);
        CHECK(r1 == 0 && r2 == 0 && b1 == b2 && b1 == 0xC000u, "34.9 deterministik");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
