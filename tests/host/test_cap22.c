/* 22.6: GERÇEK kernel/process/cap.c testi (stub değil, aynı dosya derlenir).
 * Calistirma: make test-cap22
 */
#include <stdio.h>
#include "process/cap.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    /* 22.3: bit geçerliliği */
    CHECK(cap_valid(CAP_SETUID), "22.3 tek bit gecerli");
    CHECK(!cap_valid(0), "22.3 sifir red");
    CHECK(!cap_valid(0xFFFFF000u), "22.3 maske disi red");
    CHECK(!cap_valid(CAP_SETUID | CAP_KILL), "22.3 cok bit red");

    /* 22.3: grant/revoke/has */
    {
        uint32_t s = 0;
        CHECK(cap_grant(&s, CAP_SETUID) == 0 && cap_has(s, CAP_SETUID), "22.3 grant");
        CHECK(!cap_has(s, CAP_KILL), "22.3 yok");
        CHECK(cap_revoke(&s, CAP_SETUID) == 0 && !cap_has(s, CAP_SETUID), "22.3 revoke");
        CHECK(cap_grant(NULL, CAP_SETUID) != 0, "22.3 null red");
        CHECK(cap_grant(&s, 0x80000000u) != 0, "22.3 gecersiz bit red");
        CHECK(cap_revoke(&s, 0) != 0, "22.3 sifir red");
    }

    /* 22.3: alt küme / düşürme */
    {
        uint32_t full = CAP_ALL;
        CHECK(cap_is_subset(CAP_SETUID, full), "22.3 subset");
        CHECK(!cap_is_subset(full, CAP_SETUID), "22.3 superset degil");
        CHECK(cap_allow_only(full, CAP_SETUID) == CAP_SETUID, "22.3 daraltma");
        CHECK(cap_allow_only(CAP_SETUID, CAP_ALL) == CAP_SETUID, "22.3 yukseltme red");
        uint32_t s = CAP_ALL;
        CHECK(cap_drop_all(&s) == 0 && s == 0, "22.3 drop-all");
        CHECK(cap_drop_all(NULL) != 0, "22.3 drop-null red");
    }

    /* 22.3: denetim halkası */
    {
        cap_audit(CAP_SETUID, 1);
        cap_audit(CAP_KILL, 0);
        uint32_t c, g;
        CHECK(cap_audit_read(0, &c, &g) == 0 && c == CAP_KILL && !g, "22.3 audit son");
        CHECK(cap_audit_read(1, &c, &g) == 0 && c == CAP_SETUID && g, "22.3 audit onceki");
        CHECK(cap_audit_read(99, &c, &g) != 0, "22.3 audit aralik");
        CHECK(cap_audit_read(0, NULL, &g) != 0, "22.3 audit null");
    }

    /* 22.9: priv-esc senaryoları (çekirdek politikasının saf mantık izdüşümü) */
    {
        /* root tam yetkiyle başlar */
        uint32_t caps = CAP_ALL;
        CHECK(cap_has(caps, CAP_SETUID) && cap_has(caps, CAP_SYS_RAWIO), "22.9 root tam yetki");
        /* root -> non-root geçişinde yetkiler düşer */
        cap_drop_all(&caps);
        CHECK(!cap_has(caps, CAP_SETUID), "22.9 dusunce yetki yok");
        /* düşmüş görev kendini yükseltemez */
        CHECK(cap_allow_only(caps, CAP_SETUID) == caps, "22.9 yukseltme engellendi");
        /* ham disk yazma yetkisi olmadan engellenir */
        CHECK(!cap_has(caps, CAP_SYS_RAWIO), "22.9 rawio yok");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
