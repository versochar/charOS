/* 24.5: GERÇEK kernel/core/verify.c testi (aynı dosya derlenir).
 * 24.4 entegrasyon regresyonu: cap.c/sandbox.c sözleşmeleri davranış korur.
 * Calistirma: make test-verify24
 */
#include <stdio.h>
#include "core/verify.h"
#include "process/cap.h"
#include "process/sandbox.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static uint32_t hook_code = 0;
static uint32_t hook_hits = 0;
static void rec_hook(uint32_t code, const char* file, uint32_t line) {
    (void)file; (void)line;
    hook_code = code;
    hook_hits++;
}

int main(void) {
    /* 24.3: require/hook/sayaç */
    verify_set_hook(rec_hook);
    CHECK(REQUIRE(1, 0xA001) == 0 && hook_hits == 0, "24.3 gecerli sessiz");
    CHECK(REQUIRE(0, 0xA002) == -1 && hook_hits == 1 && hook_code == 0xA002,
          "24.3 ihlal kanca+kod");
    CHECK(verify_fail_count() == 1, "24.3 sayac");
    verify_set_hook(0);
    CHECK(REQUIRE(0, 0xA003) == -1 && hook_hits == 1, "24.3 kancasiz da -1");
    CHECK(verify_fail_count() == 2, "24.3 sayac artar");

    /* 24.3: öztest */
    CHECK(verify_selftest() == 0, "24.3 selftest");

    /* 24.4: cap sözleşmeleri davranış korur + kanca kodu doğru */
    {
        uint32_t s = 0;
        verify_set_hook(rec_hook);
        hook_hits = 0;
        CHECK(cap_grant(&s, CAP_SETUID) == 0 && cap_has(s, CAP_SETUID), "24.4 grant");
        CHECK(cap_grant(&s, 0xDEADu) == -1 && hook_code == 0xC102, "24.4 grant-kod");
        CHECK(cap_revoke(NULL, CAP_SETUID) == -1 && hook_code == 0xC103, "24.4 revoke-kod");
        CHECK(cap_drop_all(NULL) == -1 && hook_code == 0xC105, "24.4 drop-kod");
        verify_set_hook(0);
    }

    /* 24.4: sandbox sözleşmeleri davranış korur */
    {
        uint32_t m[SB_MASK_WORDS];
        sb_allow_all(m);
        verify_set_hook(rec_hook);
        hook_hits = 0;
        CHECK(sb_allow(m, 7) == 0 && sb_check(m, 7), "24.4 allow");
        CHECK(sb_allow(NULL, 7) == -1 && hook_code == 0xB101, "24.4 allow-kod");
        CHECK(sb_deny(m, 999) == -1 && hook_code == 0xB104, "24.4 deny-kod");
        verify_set_hook(0);
    }

    /* 24.8/24.9: ihlal determinizmi (aynı ihlal aynı kod) */
    {
        uint32_t s = 0;
        verify_set_hook(rec_hook);
        cap_grant(&s, 0);
        uint32_t first = hook_code;
        cap_grant(&s, 0);
        CHECK(hook_code == first && first == 0xC102, "24.9 deterministik kod");
        verify_set_hook(0);
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
