/* 27.5: GERÇEK kernel/core/version.c testi (aynı dosya derlenir).
 * -D ile enjekte edilen değerlerin birebir gömüldüğü doğrulanır.
 * Calistirma: make test-version27
 */
#include <stdio.h>
#include <string.h>
#include "core/version.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    const char* s = version_string();
    const char* c = version_commit();

    /* 27.3: -D değerleri birebir gelir (Makefile test hedefindeki değerler) */
    CHECK(s && !strcmp(s, "test-27"), "27.3 string gomulu");
    CHECK(c && !strcmp(c, "abc1234"), "27.3 commit gomulu");

    /* 27.3: göstericiler salt-okunur, boş değil, tekrar çağrıda aynı */
    CHECK(s[0] != '\0' && c[0] != '\0', "27.3 bos degil");
    CHECK(version_string() == s && version_commit() == c, "27.3 kararli adres");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
