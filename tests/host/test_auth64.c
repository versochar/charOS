/* 38J: passwd64/shadow64/pam64/pammod64/sudo64/capv364/auditlog64/
 *      group64/selinuxctx64/privdrop64 host testi.
 * Calistirma: make test-auth64
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

static int deny_mod(int handle, const char *user) {
    (void)handle;
    (void)user;
    return -1;
}

int main(void) {
    u32 uid = 0, gid = 0;
    u64 h;
    int need = -1, t, p, g;
    char ctx[128];

    h = passwd64_hash("gizli", "tuz1");
    CHECK(h != 0 && h == passwd64_hash("gizli", "tuz1"), "38A deterministik");
    CHECK(h != passwd64_hash("gizli", "tuz2"), "38A tuz etkisi");
    CHECK(passwd64_add("mitsune", 1000, 1000, "/home/mitsune", "/bin/sh") ==
          0, "38A ekle");
    CHECK(passwd64_find("mitsune", &uid, &gid) == 0 && uid == 1000 &&
          gid == 1000, "38A bul");
    CHECK(passwd64_find("yok", 0, 0) != 0, "38A yok red");
    CHECK(shadow64_set("mitsune", h, 100, 90) == 0, "38A shadow");
    CHECK(shadow64_check("mitsune", h, 150) == 0, "38A giris ok");
    CHECK(shadow64_check("mitsune", h + 1, 150) != 0, "38A yanlis red");
    CHECK(shadow64_check("mitsune", h, 500) != 0, "38A sure red");

    {
        int hd = pam64_start("login", "mitsune");
        CHECK(hd >= 0, "38B baslat");
        CHECK(pam64_add(hd, pammod64_find("unix", 0), PAM64_REQUIRED) == 0,
              "38B unix modul");
        CHECK(pam64_authenticate(hd) == 0, "38B kayitli giris");
        CHECK(pam64_acct_mgmt(hd) == 0, "38B acct");
        pam64_end(hd);
        hd = pam64_start("login", "hayalet");
        pam64_add(hd, pammod64_find("unix", 0), PAM64_REQUIRED);
        CHECK(pam64_authenticate(hd) != 0, "38B kayitsiz red");
        pam64_end(hd);
        hd = pam64_start("login", "mitsune");
        pam64_add(hd, deny_mod, PAM64_SUFFICIENT);
        pam64_add(hd, pammod64_find("permit", 0), PAM64_REQUIRED);
        CHECK(pam64_authenticate(hd) == 0, "38B sufficient atlama");
        pam64_end(hd);
        hd = pam64_start("login", "mitsune");
        pam64_add(hd, deny_mod, PAM64_REQUISITE);
        pam64_add(hd, pammod64_find("permit", 0), PAM64_REQUIRED);
        CHECK(pam64_authenticate(hd) != 0, "38B requisite durdurur");
        pam64_end(hd);
        CHECK(pammod64_find("yok", 0) == 0, "38H bilinmeyen red");
    }

    CHECK(sudo64_rule_add("mitsune", "root", "*", 0) == 0, "38C kural");
    CHECK(sudo64_check("mitsune", "root", "/bin/ls", &need) == 1 &&
          need == 1, "38C joker+sifre");
    CHECK(sudo64_rule_add("mitsune", "root", "/bin/ls", 1) == 0,
          "38C ozel kural");
    CHECK(sudo64_check("mitsune", "root", "/bin/ls", &need) == 1,
          "38C ilk eslesme kazanir");
    CHECK(sudo64_check("yabanci", "root", "/bin/ls", 0) == 0,
          "38C yetkisiz red");
    CHECK(sudo64_stamp("mitsune", 1000) == 0, "38C damga");
    CHECK(sudo64_stamp_valid("mitsune", 1300, 600) == 1, "38C gecerli");
    CHECK(sudo64_stamp_valid("mitsune", 2000, 600) == 0, "38C zamanaşımı");

    CHECK(capv364_set(0, CAPV364_EFFECTIVE, 0xFF) == 0, "38D kume");
    CHECK(capv364_has(0, CAPV364_EFFECTIVE, 3) == 1, "38D sorgu");
    CHECK(capv364_bound_add(~0xFULL) == 0, "38D sinir daralt");
    CHECK(capv364_bound_get() == ~0xFULL, "38D sinir oku");
    CHECK(capv364_set(1, CAPV364_PERMITTED, ~0ULL) == 0, "38D sinirli ata");
    CHECK(capv364_get(1, CAPV364_PERMITTED) == ~0xFULL,
          "38D sinir uygulanir");
    CHECK(capv364_exec(0, 0xF0, 0x0F) == 0, "38D exec gecis");
    CHECK(capv364_has(0, CAPV364_PERMITTED, 100) == 0, "38D aralik disi");

    CHECK(auditlog64_write(1, 100, "giris") == 0, "38E yaz");
    CHECK(auditlog64_write(2, 101, "cikis") == 0, "38E yaz2");
    CHECK(auditlog64_count_type(1) == 1, "38E tur sayac");
    {
        u64 seq = auditlog64_next_seq() - 1;
        CHECK(auditlog64_read(seq, &t, &p, ctx, sizeof(ctx)) == 0 &&
              t == 2 && p == 101 && !strcmp(ctx, "cikis"), "38E oku");
        CHECK(auditlog64_read(999999, 0, 0, 0, 0) != 0, "38E yok red");
    }

    CHECK(group64_add("wheel", 10) == 0, "38F grup");
    CHECK(group64_add_member("wheel", "mitsune") == 0, "38F uye");
    CHECK(group64_is_member("wheel", "mitsune"), "38F uyelik");
    CHECK(!group64_is_member("wheel", "yabanci"), "38F disi");
    CHECK(group64_policy_add("wheel", "/etc/shadow", 4) == 0,
          "38F politika");
    CHECK(group64_check("mitsune", "/etc/shadow", 4) == 1, "38F erisim");
    CHECK(group64_check("yabanci", "/etc/shadow", 4) == 0, "38F red");
    CHECK(group64_check("mitsune", "/etc/shadow", 2) == 0, "38F yetki red");

    CHECK(selinuxctx64_add("/bin/", "u:r:bin:s0") == 0, "38G etiket");
    CHECK(selinuxctx64_add("/bin/ls", "u:r:ls:s0") == 0, "38G ozel");
    CHECK(selinuxctx64_label("/bin/ls", ctx, sizeof(ctx)) == 0 &&
          !strcmp(ctx, "u:r:ls:s0"), "38G en uzun kazanir");
    CHECK(selinuxctx64_label("/yok", ctx, sizeof(ctx)) != 0,
          "38G etiketsiz red");
    CHECK(selinuxctx64_add_trans("init", "bin", "file", "u:r:bin_child:s0") ==
          0, "38G gecis kurali");
    CHECK(selinuxctx64_trans("init", "bin", "file", ctx, sizeof(ctx)) == 0 &&
          !strcmp(ctx, "u:r:bin_child:s0"), "38G gecis");
    CHECK(selinuxctx64_trans("x", "bin", "file", ctx, sizeof(ctx)) != 0,
          "38G gecis yok");

    CHECK(privdrop64_set(0, 0, 0, 0) == 0, "38I root kur");
    CHECK(privdrop64_is_root(), "38I root");
    CHECK(privdrop64_drop(1000, 1000) == 0, "38I dusur");
    CHECK(!privdrop64_is_root(), "38I artik root degil");
    CHECK(!privdrop64_dumpable(), "38I dump kapali");
    CHECK(privdrop64_drop(0, 0) != 0, "38I geri donus yok");
    (void)g;

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
