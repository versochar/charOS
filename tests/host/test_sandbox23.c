/* 23.6: GERÇEK kernel/process/sandbox.c testi (stub değil, aynı dosya derlenir).
 * Calistirma: make test-sandbox23
 */
#include <stdio.h>
#include "process/sandbox.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    uint32_t m[SB_MASK_WORDS];

    /* 23.3: allow-all / lockdown */
    sb_allow_all(m);
    CHECK(sb_check(m, 0) && sb_check(m, 162) && sb_check(m, 255), "23.3 allow-all");
    sb_lockdown(m);
    CHECK(!sb_check(m, 0) && !sb_check(m, 255), "23.3 lockdown");

    /* 23.3: allow/deny */
    CHECK(sb_allow(m, 10) == 0 && sb_check(m, 10), "23.3 allow");
    CHECK(sb_deny(m, 10) == 0 && !sb_check(m, 10), "23.3 deny");
    CHECK(sb_allow(m, 256) != 0, "23.3 nr-aralik");
    CHECK(sb_deny(NULL, 1) != 0, "23.3 null red");
    CHECK(sb_allow(NULL, 1) != 0, "23.3 null red2");
    CHECK(!sb_check(NULL, 1), "23.3 null deny");
    CHECK(!sb_check(m, 999), "23.3 aralik-disi deny");

    /* 23.5: tipik kilitleme senaryosu (handler mantığının izdüşümü) */
    {
        uint32_t t[SB_MASK_WORDS];
        sb_allow_all(t);
        sb_deny(t, 132); /* SYS_BLKWRITE */
        CHECK(sb_check(t, 1), "23.5 write serbest");
        CHECK(!sb_check(t, 132), "23.5 blkwrite engelli");
    }

    /* 23.9: sandbox kaçış senaryoları */
    {
        uint32_t t[SB_MASK_WORDS];
        sb_lockdown(t);
        /* kilitli görev kendini serbest bırakamaz (maske handler dışında değişmez;
         * burada SB_ALLOW yolu simüle edilir: allow tek tek açar) */
        CHECK(sb_allow(t, 4) == 0 && sb_check(t, 4), "23.9 secici-acma");
        CHECK(!sb_check(t, 5), "23.9 digerleri kapali");
        /* maske sınırları */
        CHECK(sb_allow(t, 255) == 0 && sb_check(t, 255), "23.9 sinir-255");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
