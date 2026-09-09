/* 53J: prelink64/ldpath64/nss64/iconvprov64/cryptoprov64/plugin64/
 *      hotplug64/selftest64/linktest64 host testi.
 * Calistirma: make test-link64
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

static int upcase(const unsigned char *in, u64 ilen, unsigned char *out,
                  u64 *olen) {
    u64 i;
    if (!in || !out || !olen || ilen > *olen) return -1;
    for (i = 0; i < ilen; i++) {
        unsigned char c = in[i];
        out[i] = (c >= 'a' && c <= 'z') ? (unsigned char)(c - 32) : c;
    }
    *olen = ilen;
    return 0;
}

static int st_ok(void) { return 0; }
static int st_bad(void) { return 3; }

static int plug_entry(int op, u64 arg) {
    (void)arg;
    if (op == 0 || op == 2) return 0; /* init + ozel op */
    if (op == 1) return 0;            /* fini */
    return -1;
}

int main(void) {
    const char *libs[3] = {"libc", "libm", "libx"};
    const u64 sizes[3] = {0x100000, 0x80000, 0x40000};
    u64 addrs[3] = {0, 0, 0};
    char dirs[4][96];
    char exp[128], found[128];
    int n;

    CHECK(prelink64_assign(libs, sizes, 3, 0x40000000ULL, addrs) == 0 &&
          addrs[0] == 0x40000000ULL, "53A ata");
    CHECK(addrs[1] >= addrs[0] + 0x100000 &&
          (addrs[1] & 0x1FFFFFULL) == 0, "53A hiza+aralik");
    CHECK(prelink64_verify(addrs, sizes, 3) == 0, "53A cakismasiz");
    {
        u64 bad_a[2] = {0x1000, 0x1800};
        u64 bad_s[2] = {0x1000, 0x1000};
        CHECK(prelink64_verify(bad_a, bad_s, 2) == 1, "53A cakisma yeri");
    }
    CHECK(prelink64_assign(libs, sizes, 3, 0x40000001ULL, addrs) != 0,
          "53A hizasiz taban red");

    CHECK(ldpath64_parse("/a:/b::/c:", dirs, 4) == 3 &&
          !strcmp(dirs[2], "/c"), "53B ayristir");
    CHECK(ldpath64_expand("$ORIGIN/lib", "/opt/app", exp, sizeof(exp)) ==
              0 && !strcmp(exp, "/opt/app/lib"), "53B origin");
    CHECK(ldpath64_expand("${ORIGIN}/lib", "/o", exp, sizeof(exp)) == 0 &&
          !strcmp(exp, "/o/lib"), "53B suslu");
    CHECK(rpath64_add_file("/a", "libt.so") == 0, "53B dosya");
    CHECK(ldpath64_find("/a:/b", "libt.so", found, sizeof(found)) == 0 &&
          !strcmp(found, "/a/libt.so"), "53B bul");
    CHECK(ldpath64_find("/b", "libt.so", 0, 0) != 0, "53B yok red");
    n = rpath64_add_file("/x", "libt.so");
    (void)n;

    CHECK(nss64_add("hosts", "files") == 0, "53C servis");
    CHECK(nss64_add("hosts", "dns") == 0, "53C servis2");
    CHECK(nss64_module_add("hosts", "files", "yazici", "192.168.1.20") ==
          0, "53C kayit");
    CHECK(nss64_module_add("hosts", "dns", "a.b", "1.2.3.4") == 0,
          "53C kayit2");
    CHECK(nss64_lookup("hosts", "yazici", found, sizeof(found)) == 0 &&
          !strcmp(found, "192.168.1.20"), "53C sira: files once");
    CHECK(nss64_lookup("hosts", "yok", 0, 0) != 0, "53C yok red");
    CHECK(nss64_lookup("yokdb", "x", 0, 0) != 0, "53C servis yok");

    {
        unsigned char o[16];
        u64 ol = sizeof(o);
        CHECK(iconvprov64_register("lower", "upper", upcase) == 0,
              "53D kayit");
        CHECK(iconvprov64_convert("lower", "upper",
                                  (const unsigned char *)"abc", 3, o,
                                  &ol) == 0 && ol == 3 && o[0] == 'A',
              "53D donustur");
        CHECK(iconvprov64_convert("a", "b", o, 1, o, &ol) != 0,
              "53D saglayici yok");
    }

    {
        u64 h = 0;
        unsigned char key[32], enc[16], dec[16];
        int i;
        for (i = 0; i < 32; i++) key[i] = (unsigned char)(i + 1);
        CHECK(cryptoprov64_hash("fnv1a64", "abc", 3, &h) == 0 && h,
              "53E karma");
        CHECK(cryptoprov64_hash("yok", "a", 1, 0) != 0,
              "53E algoritma yok");
        CHECK(cryptoprov64_crypt("xorstream", key,
                                 (const unsigned char *)"merhaba!",
                                 enc, 8) == 0, "53E sifrele");
        CHECK(cryptoprov64_crypt("xorstream", key, enc, dec, 8) == 0 &&
              !memcmp(dec, "merhaba!", 8), "53E coz");
    }

    CHECK(plugin64_load("ses", "1.0", plug_entry) == 0, "53F yukle");
    CHECK(plugin64_call("ses", 2, 0) == 0, "53F cagri");
    CHECK(plugin64_call("yok", 0, 0) != 0, "53F yok red");
    CHECK(plugin64_add_dep("ses", "kodek") == 0, "53F bagimlilik");
    CHECK(plugin64_check_deps("ses") == 1, "53F eksik");
    CHECK(plugin64_load("kodek", "1.0", plug_entry) == 0, "53F yukle2");
    CHECK(plugin64_check_deps("ses") == 0, "53F tamam");
    CHECK(plugin64_unload("ses") == 0, "53F kaldir");
    CHECK(plugin64_call("ses", 0, 0) != 0, "53F kaldir sonrasi");

    CHECK(hotplug64_insert("snd0") == 0, "53G tak");
    CHECK(hotplug64_insert("snd0") == 0, "53G cift sayac");
    CHECK(hotplug64_remove("snd0") != 0, "53G kullanimda red");
    CHECK(hotplug64_remove("snd0") == 0, "53G cikar");
    CHECK(hotplug64_event("usb1", 0) == 0, "53G olay");
    {
        char dev[48];
        int act = -1;
        CHECK(hotplug64_poll(dev, sizeof(dev), &act) == 0 &&
              !strcmp(dev, "usb1") && act == 0, "53G yokla");
        CHECK(hotplug64_poll(0, 0, 0) != 0, "53G bos red");
    }

    CHECK(selftest64_add("a-ok", st_ok) == 0, "53H ekle");
    CHECK(selftest64_add("b-bozuk", st_bad) == 0, "53H ekle2");
    CHECK(selftest64_run() == 1, "53H 1 basarisiz");
    {
        char nm[48];
        CHECK(selftest64_result(1, nm, sizeof(nm)) == 3 &&
              !strcmp(nm, "b-bozuk"), "53H sonuc");
        CHECK(selftest64_result(99, 0, 0) != 0, "53H aralik red");
    }

    CHECK(delayload64_register("guncel_fonksiyon") == 0, "53I kayit");
    {
        const char *syms[2] = {"guncel_fonksiyon", "kayip_fonksiyon"};
        CHECK(linktest64_undef(syms, 2) == 2, "53I ikisi de cozumsuz");
        CHECK(delayload64_resolve("guncel_fonksiyon", 0x7777) == 0,
              "53I coz");
        CHECK(linktest64_undef(syms, 2) == 1, "53I biri kaldi");
    }
    CHECK(linktest64_reloc(0x400000ULL) == 0, "53I hizali taban");
    CHECK(linktest64_reloc(0x400001ULL) != 0, "53I hizasiz red");
    CHECK(linktest64_reloc(0) != 0, "53I sifir red");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
