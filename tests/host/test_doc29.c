/* 29.5: GERÇEK kernel/core/doc.c testi (aynı dosya derlenir).
 * Calistirma: make test-doc29
 */
#include <stdio.h>
#include <string.h>
#include "core/doc.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    /* 29.3: sayı + bilinen kayıtlar */
    CHECK(doc_count() > 50, "29.3 sayi");
    CHECK(doc_name(1) && !strcmp(doc_name(1), "WRITE"), "29.3 ad");
    CHECK(doc_desc(141) && doc_desc(141)[0] != '\0', "29.3 aciklama");
    CHECK(doc_name(999) == 0 && doc_desc(999) == 0, "29.3 tanimsiz-null");

    /* 29.3: yeni syscall'lar belgeli */
    CHECK(doc_name(166) && doc_name(167), "29.3 doc-kendisi-belgeli");

    /* 29.3: sınırlı kopya */
    {
        char b[16];
        CHECK(doc_copy("WRITE", b, sizeof(b)) == 5 && !strcmp(b, "WRITE"),
              "29.3 kopya");
        CHECK(doc_copy("WRITE", b, 3) == -1, "29.3 tasma red");
        CHECK(doc_copy(NULL, b, sizeof(b)) == -1, "29.3 null red");
        CHECK(doc_copy("x", NULL, 4) == -1, "29.3 null-out red");
        CHECK(doc_copy("x", b, 0) == -1, "29.3 sifir red");
    }

    /* 29.3: öztest */
    CHECK(doc_selftest() == 0, "29.3 selftest");

    /* 29.9: örneklem tutarlılığı (ad+acıklama dolu, gösterici kararlı) */
    {
        uint32_t sample[] = {0, 12, 90, 130, 141, 161, 166, 167};
        int ok = 1;
        for (int i = 0; i < 8; i++) {
            const char* a = doc_name(sample[i]);
            const char* b = doc_desc(sample[i]);
            if (!a || !a[0] || !b || !b[0]) ok = 0;
            if (doc_name(sample[i]) != a) ok = 0; /* kararlı gösterici */
        }
        CHECK(ok, "29.9 orneklem");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
