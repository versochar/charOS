/* 41J: charpkg64/repo64/depsolve64/sign64/txn64/upgrade64/mirror64/
 *      flatpak64/manifest64 host testi.
 * Calistirma: make test-pkg64
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

static unsigned char pkgbuf[512];

static void wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static void wr64(unsigned char *p, u64 v) {
    int i;
    for (i = 0; i < 8; i++) p[i] = (unsigned char)((v >> (i * 8)) & 0xFF);
}

/* Sentetik .char: shell 1.0 x86_64, 2 dosya (veri 256. bayttan sonra) */
static void build_pkg(void) {
    unsigned char *e;
    memset(pkgbuf, 0, sizeof(pkgbuf));
    wr32(pkgbuf + 0, 0x52414843UL);
    memcpy(pkgbuf + 4, "shell", 6);
    memcpy(pkgbuf + 36, "1.0", 4);
    memcpy(pkgbuf + 52, "x86_64", 7);
    wr32(pkgbuf + 60, 2);
    e = pkgbuf + 64;
    memcpy(e + 0, "/bin/sh", 8);
    wr64(e + 64, 240);
    wr64(e + 72, 100);
    wr64(e + 80, 0x1234);
    e = pkgbuf + 64 + 88;
    memcpy(e + 0, "/etc/shell.rc", 13);
    wr64(e + 64, 340);
    wr64(e + 72, 50);
    wr64(e + 80, 0x5678);
    memset(pkgbuf + 240, 0x41, 100);
    memset(pkgbuf + 340, 0x42, 50);
}

int main(void) {
    struct charpkg64_hdr hdr;
    struct charpkg64_file files[4];
    int n = 0, i;
    u64 off = 0, sz = 0;
    char ver[16], pick[96], fver[16];
    char order[8][32];
    struct depsolve64_dep dep;
    struct flatpak64_ref ref;
    struct manifest64 man;

    build_pkg();
    CHECK(charpkg64_parse(pkgbuf, sizeof(pkgbuf), &hdr, files, 4, &n) == 0 &&
          n == 2, "41A ayristir");
    CHECK(!strcmp(hdr.name, "shell") && !strcmp(hdr.ver, "1.0") &&
          hdr.nfiles == 2, "41A baslik");
    CHECK(charpkg64_find(files, n, "/bin/sh", &off, &sz) == 0 &&
          off == 240 && sz == 100, "41A dosya bul");
    CHECK(charpkg64_find(files, n, "/yok", 0, 0) != 0, "41A yok red");
    CHECK(charpkg64_parse(pkgbuf, 10, 0, 0, 0, 0) != 0, "41A kisa red");
    CHECK(charpkg64_parse(pkgbuf, sizeof(pkgbuf), 0, files, 1, &n) != 0,
          "41A tasma red");

    CHECK(repo64_vercmp("1.10.0", "1.2.0") > 0, "41B sayisal surum");
    CHECK(repo64_vercmp("2.0", "2.0") == 0, "41B esit");
    CHECK(repo64_vercmp("1.2a", "1.2") > 0, "41B sonek");
    CHECK(repo64_vercmp("1.0", "2.0") < 0, "41B kucuk");
    CHECK(repo64_add("shell", "1.0", "core") == 0, "41B ekle");
    CHECK(repo64_add("shell", "2.0", "core") == 0, "41B ekle2");
    CHECK(repo64_find("shell", 0, ver, sizeof(ver)) == 0 &&
          !strcmp(ver, "2.0"), "41B en yuksek");
    CHECK(repo64_find("yok", 0, 0, 0) != 0, "41B yok red");

    CHECK(depsolve64_parse_dep("libc>=2.1", &dep) == 0 &&
          !strcmp(dep.name, "libc") && dep.op == 1 &&
          !strcmp(dep.ver, "2.1"), "41C ayristir");
    CHECK(depsolve64_parse_dep("tek", &dep) == 0 && dep.op == 0,
          "41C surumsuz");
    {
        char dd0[2][48] = {"libc>=2.0", "zlib"};
        char dd1[1][48] = {""};
        CHECK(depsolve64_add_meta("uygulama", "1.0", dd0, 2) == 0,
              "41C meta");
        CHECK(depsolve64_add_meta("libc", "2.1", dd1, 0) == 0,
              "41C meta2");
        CHECK(depsolve64_add_meta("zlib", "1.3", dd1, 0) == 0,
              "41C meta3");
        repo64_add("libc", "2.1", "core");
        repo64_add("zlib", "1.3", "core");
        i = depsolve64_solve("uygulama", order, 8);
        CHECK(i == 3, "41C cozum adedi");
        CHECK(!strcmp(order[2], "uygulama"), "41C sira: kok en son");
        CHECK(depsolve64_solve("yokpaket", order, 8) != 0,
              "41C bilinmeyen red");
    }
    {
        char dd[1][48] = {"b"};
        char dd2[1][48] = {"a"};
        depsolve64_add_meta("a", "1.0", dd, 1);
        depsolve64_add_meta("b", "1.0", dd2, 1);
        CHECK(depsolve64_solve("a", order, 8) != 0, "41C dongu red");
    }

    {
        const char data[] = "paket-icerigi";
        u64 sig;
        CHECK(sign64_add_key(7, 0xBEEF) == 0, "41D anahtar");
        sig = sign64_sign(data, sizeof(data), 7);
        CHECK(sig != 0, "41D imzala");
        CHECK(sign64_verify(data, sizeof(data), sig, 7) == 0,
              "41D dogrula");
        CHECK(sign64_verify("sahte!", 6, sig, 7) != 0, "41D sahte red");
        CHECK(sign64_verify(data, sizeof(data), sig, 99) != 0,
              "41D anahtar yok");
    }

    CHECK(txn64_begin() == 0, "41E baslat");
    CHECK(txn64_install("shell", "2.0") == 0, "41E kur");
    CHECK(txn64_install("top", "1.0") == 0, "41E kur2");
    CHECK(txn64_commit() == 0, "41E commit");
    CHECK(txn64_installed("shell", ver, sizeof(ver)) == 0 &&
          !strcmp(ver, "2.0"), "41E kurulu");
    CHECK(txn64_begin() == 0, "41E baslat2");
    CHECK(txn64_install("shell", "3.0") == 0, "41E yukselt");
    CHECK(txn64_remove("top") == 0, "41E kaldir");
    CHECK(txn64_rollback() == 0, "41E geri al");
    CHECK(txn64_installed("shell", ver, sizeof(ver)) == 0 &&
          !strcmp(ver, "2.0"), "41E eski surum duruyor");
    CHECK(txn64_installed("top", 0, 0) == 0, "41E kaldirilan duruyor");
    CHECK(txn64_begin() == 0, "41E baslat3");
    CHECK(txn64_remove("yok") != 0, "41E kurulu-olmayan red");
    CHECK(txn64_rollback() == 0, "41E bos geri al");

    CHECK(upgrade64_stage("v2") == 0, "41F hazirla");
    CHECK(upgrade64_commit() == 0 && upgrade64_active() == 1,
          "41F cevir");
    CHECK(upgrade64_version(1, ver, sizeof(ver)) == 0 &&
          !strcmp(ver, "v2"), "41F surum oku");
    CHECK(upgrade64_commit() != 0, "41F hazirliksiz red");
    CHECK(upgrade64_rollback() != 0, "41F bos slota donus yok");
    CHECK(upgrade64_stage("v3") == 0 && upgrade64_commit() == 0,
          "41F ikinci cevir");
    CHECK(upgrade64_rollback() == 0 && upgrade64_active() == 1,
          "41F geri al");

    CHECK(mirror64_add("https://a.ornek", 10) == 0, "41G ekle");
    CHECK(mirror64_add("https://b.ornek", 20) == 0, "41G ekle2");
    CHECK(mirror64_pick(pick, sizeof(pick)) == 0 &&
          !strcmp(pick, "https://b.ornek"), "41G oncelik");
    CHECK(mirror64_fail("https://b.ornek") == 0, "41G dusur");
    CHECK(mirror64_pick(pick, sizeof(pick)) == 0 &&
          !strcmp(pick, "https://a.ornek"), "41G yedek");
    CHECK(mirror64_sync_ok("https://b.ornek", 5000) == 0, "41G eszaman");
    CHECK(mirror64_pick(pick, sizeof(pick)) == 0 &&
          !strcmp(pick, "https://b.ornek"), "41G geri donus");

    CHECK(flatpak64_parse_ref("runtime/org.gnome.Plan/x86_64/stable",
                              &ref) == 0, "41H ref");
    CHECK(!strcmp(ref.kind, "runtime") && !strcmp(ref.name, "org.gnome.Plan") &&
          !strcmp(ref.arch, "x86_64") && !strcmp(ref.branch, "stable"),
          "41H alanlar");
    CHECK(flatpak64_parse_ref("org.gnome.Plan/x86_64/stable", &ref) == 0,
          "41H tursuz ref");
    CHECK(flatpak64_parse_ref("bozuk", &ref) != 0, "41H bozuk red");
    CHECK(flatpak64_add_runtime("org.gnome.Plan", "x86_64", "stable",
                                "44") == 0, "41H calisma zamani");
    CHECK(flatpak64_find_runtime("org.gnome.Plan", "x86_64", "stable",
                                 fver, sizeof(fver)) == 0 &&
          !strcmp(fver, "44"), "41H bul");
    CHECK(flatpak64_find_runtime("yok", "x86_64", "stable", 0, 0) != 0,
          "41H yok red");

    {
        const char *mf = "# ornek\nname=shell-flat\nruntime=runtime/"
                         "org.gnome.Plan/x86_64/stable\nbranch=stable\n"
                         "command=shell\nbilinmeyen=atlanir\n";
        CHECK(manifest64_parse(mf, &man) == 0, "41I ayristir");
        CHECK(!strcmp(man.name, "shell-flat") &&
              !strcmp(man.branch, "stable") &&
              !strcmp(man.command, "shell"), "41I alanlar");
        CHECK(manifest64_parse("runtime=x\n", &man) != 0,
              "41I adsiz red");
        CHECK(manifest64_parse(mf, &man) == 0 &&
              manifest64_to_pkg(&man) == 0, "41I pakete donustur");
        CHECK(repo64_find("shell-flat", 0, ver, sizeof(ver)) == 0,
              "41I repo kaydi");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
