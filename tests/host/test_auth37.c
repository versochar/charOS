/* 37.5: GERÇEK kernel/core/auth.c testi (aynı dosya derlenir).
 * Token türetimi + doğrulama karşılaştırması.
 */
#include <stdio.h>
#include <string.h>
#include "core/auth.h"
#include "process/cap.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    uint32_t tok;

    auth_init();
    CHECK(auth_check("x", 0) == -1, "37.3 kapali");

    tok = auth_token_gen("host", 0xDEADBEEF);
    CHECK(tok != 0, "37.3 bos-degil");
    CHECK(tok == auth_token_gen("host", 0xDEADBEEF), "37.3 deterministik");

    CHECK(auth_check("host", tok) == AUTH_OK, "37.3 dogru");
    CHECK(auth_check("host", tok ^ 1) == -1, "37.3 bozuk");
    CHECK(auth_check("bad", tok) == -1, "37.3 isim-yanlis");
    CHECK(auth_check("", tok) == -1, "37.3 bos-isim");
    CHECK(auth_token_gen(NULL, 0) == 0, "37.3 null-isim");
    CHECK(auth_check(NULL, tok) == -1, "37.3 null-isim-check");

    /* 37.7: audit günlüğü */
    cap_audit(CAP_DAC_OVERRIDE, 1);
    cap_audit(CAP_KILL, 0);
    uint32_t cap, granted;
    CHECK(cap_audit_read(0, &cap, &granted) == 0 &&
          ((cap == CAP_KILL) || (cap == CAP_DAC_OVERRIDE)) &&
          ((granted == 1 && cap == CAP_DAC_OVERRIDE) ||
           (granted == 0 && cap == CAP_KILL)), "37.7 audit");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
