/* 30.5: GERÇEK kernel/core/abi.c testi (aynı dosya derlenir).
 * Calistirma: make test-abi30
 */
#include <stdio.h>
#include "core/abi.h"
#include "core/doc.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    /* 30.3: mühür korunur */
    CHECK(abi_frozen_check() == 0, "30.3 muhur");
    CHECK(abi_selftest() == 0, "30.3 selftest");

    /* 30.3: mühür kritik numaraları kapsar */
    CHECK(doc_count() >= 8, "30.3 sayi");
    CHECK(doc_name(12) != 0 && doc_name(141) != 0, "30.3 kritik");

    /* 30.9: deterministik tekrar */
    CHECK(abi_frozen_check() == 0 && abi_selftest() == 0, "30.9 deterministik");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
