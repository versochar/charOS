/* 37J: aslr64/nx64/scanary64/seccomp64/capaudit64/selinux64/apparmor64/
 *      userns64/modsign64 host testi (pen-test senaryolari dahil).
 * Calistirma: make test-sec64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int pid, cap, gr;
    struct selinux64_ctx ctx;

    CHECK((aslr64_stack_base(0x1234) & 0xF) == 0, "37A yigin hiza");
    CHECK(aslr64_stack_base(0x1234) != aslr64_stack_base(0x5678),
          "37A yigin dagilim");
    CHECK((aslr64_mmap_base(0x1234) & 0x1FFFFFULL) == 0, "37A mmap hiza");
    CHECK(aslr64_exec_base(0x1234, 0) == 0x1000000ULL, "37A sabit exec");
    CHECK(aslr64_exec_base(0x1234, 1) != aslr64_exec_base(0x5678, 1),
          "37A pie dagilim");

    CHECK(nx64_enforce(NX64_R | NX64_W | NX64_X, 1) ==
          (NX64_R | NX64_W), "37B yigin X temizlenir");
    CHECK(nx64_enforce(NX64_R | NX64_X, 0) == (NX64_R | NX64_X),
          "37B kod X korunur");
    CHECK(!nx64_exec_allowed(nx64_stack_flags()), "37B yigin calismaz");

    {
        u64 c = scanary64_gen(0xDEAD);
        CHECK(c != 0, "37C sifir degil");
        CHECK(scanary64_verify(c, c) == 0, "37C eslesme");
        CHECK(scanary64_verify(c, c ^ 1) != 0, "37C tasma yakalama");
        CHECK(scanary64_set_cpu(0, c) == 0 &&
              scanary64_get_cpu(0) == c, "37C per-cpu");
        CHECK(scanary64_set_cpu(99, c) != 0, "37C cpu red");
    }

    seccomp64_strict(0);
    CHECK(seccomp64_check(999) == 1, "37D varsayilan serbest");
    CHECK(seccomp64_add(59, 0) == 0, "37D kural ekle");
    CHECK(seccomp64_check(59) == 0, "37D engelli");
    seccomp64_strict(1);
    CHECK(seccomp64_check(999) == 0, "37D kati engel");
    CHECK(seccomp64_check(59) == 0, "37D kural korunur");
    CHECK(seccomp64_count() == 1, "37D sayac");
    seccomp64_strict(0);

    CHECK(capaudit64_grant(7) == 0, "37E ver");
    CHECK(capaudit64_has(7), "37E sorgu");
    CHECK(capaudit64_revoke(7) == 0 && !capaudit64_has(7), "37E geri al");
    CHECK(capaudit64_grant(99) != 0, "37E aralik red");
    capaudit64_log(100, 7, 1);
    capaudit64_log(101, 8, 0);
    CHECK(capaudit64_read(0, &pid, &cap, &gr) == 0 && pid == 101 &&
          cap == 8 && !gr, "37E son kayit");
    CHECK(capaudit64_read(1, &pid, &cap, &gr) == 0 && pid == 100,
          "37E onceki kayit");

    CHECK(selinux64_parse("u:r:init:s0", &ctx) == 0, "37F ayristir");
    CHECK(!strcmp(ctx.user, "u") && !strcmp(ctx.type, "init"),
          "37F alanlar");
    CHECK(selinux64_parse("eksik", &ctx) != 0, "37F bozuk red");
    CHECK(selinux64_add_rule("init", "bin", "file", 0x4) == 0,
          "37F kural");
    CHECK(selinux64_check("init", "bin", "file", 0x4) == 1, "37F izin");
    CHECK(selinux64_check("init", "bin", "file", 0x2) == 0, "37F yetki red");
    CHECK(selinux64_check("x", "bin", "file", 0x4) == 0, "37F kural yok");
    selinux64_enforcing(0);
    CHECK(!selinux64_is_enforcing(), "37F permissive");
    selinux64_enforcing(1);

    CHECK(apparmor64_add_profile("/usr/bin/", 1) == 0, "37G profil");
    CHECK(apparmor64_add_rule("/usr/bin/", "/usr/bin/top", APPARMOR64_R) ==
          0, "37G kural");
    CHECK(apparmor64_check("/usr/bin/top", APPARMOR64_R) == 1,
          "37G okuma serbest");
    CHECK(apparmor64_check("/usr/bin/top", APPARMOR64_W) == 0,
          "37G yazma engelli");
    CHECK(apparmor64_check("/etc/passwd", APPARMOR64_R) == 1,
          "37G profilsiz serbest");

    {
        u32 mapped = 0;
        CHECK(userns64_add_map(1000, 0, 100) == 0, "37H esleme");
        CHECK(userns64_map_uid(1050, &mapped) == 0 && mapped == 50,
              "37H ceviri");
        CHECK(userns64_map_uid(5000, &mapped) != 0, "37H disi red");
        CHECK(userns64_level() == 0, "37H seviye0");
        userns64_enter();
        CHECK(userns64_level() == 1, "37H ic ice");
    }

    {
        const char mod[] = "charos-test-modulu";
        u64 h = modsign64_hash(mod, sizeof(mod));
        CHECK(h != 0, "37I karma");
        CHECK(modsign64_add_key(1, h) == 0, "37I anahtar");
        CHECK(modsign64_verify(mod, sizeof(mod), 1) == 0, "37I dogrulama");
        CHECK(modsign64_verify("sahte", 5, 1) != 0, "37I sahte red");
        CHECK(modsign64_tainted(), "37I taint");
        CHECK(modsign64_verify(mod, sizeof(mod), 99) != 0,
              "37I anahtar yok");
    }

    /* Pen-test senaryolari */
    CHECK(nx64_exec_allowed(NX64_R | NX64_W) == 0, "37J yigin calistirma");
    CHECK(scanary64_verify(1, 2) != 0, "37J canary bypass");
    seccomp64_strict(1);
    CHECK(seccomp64_check(1) == 0, "37J kati liste");
    seccomp64_strict(0);
    CHECK(selinux64_check("a", "b", "c", 1) == 0, "37J selinux default-deny");
    CHECK(apparmor64_check("/usr/bin/top", 99) != 1, "37J op red");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
