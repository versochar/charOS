/* 54J: shell64/coreutils64/busybox64/textutils64/sshclient64/gitclient64/
 *      make64/python64/node64 host testi.
 * Calistirma: make test-user64
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
    char toks[8][64];
    char exp[64], ob[128];
    int n;

    CHECK(shell64_tokenize("ls -l \"/tmp/a b\" # yorum", toks, 8) == 3 &&
          !strcmp(toks[0], "ls") && !strcmp(toks[2], "/tmp/a b"),
          "54A jeton");
    CHECK(shell64_tokenize("echo 'tek tirnak'", toks, 8) == 2 &&
          !strcmp(toks[1], "tek tirnak"), "54A tirnak");
    CHECK(shell64_expand("ev:$EV", "EV", "/ev", exp, sizeof(exp)) == 0 &&
          !strcmp(exp, "ev:/ev"), "54A genislet");
    CHECK(shell64_expand("a${EV}b", "EV", "X", exp, sizeof(exp)) == 0 &&
          !strcmp(exp, "aXb"), "54A suslu");
    {
        char av[2][64] = {"-n", "merhaba"};
        CHECK(shell64_builtin("echo", 2, av, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "merhaba"), "54A echo -n");
        CHECK(shell64_builtin("bogus", 0, 0, 0, 0) == 127,
              "54A bilinmeyen");
        CHECK(shell64_builtin("false", 0, 0, 0, 0) == 1, "54A false");
    }
    CHECK(shell64_history_add("ls") == 0, "54A gecmis");
    CHECK(shell64_history_get(0, ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "ls"), "54A gecmis oku");

    CHECK(coreutils64_exists("ls") && coreutils64_exists("sort"),
          "54B kayitli");
    CHECK(!coreutils64_exists("bogus"), "54B yok");
    {
        char lst[8][32];
        CHECK(coreutils64_list(0, lst, 8) == 8, "54B liste");
    }
    {
        u64 l = 0, w = 0, b = 0;
        CHECK(coreutils64_wc("a bb\nccc\n", &l, &w, &b) == 0 && l == 2 &&
              w == 3 && b == 9, "54B wc");
    }

    {
        char av[1][64] = {"selam"};
        CHECK(busybox64_applet("echo", 1, av, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "selam\n"), "54C echo");
        CHECK(busybox64_applet("yok-applet", 0, 0, 0, 0) == 127,
              "54C yok red");
    }

    CHECK(textutils64_grep("hata", "ok\nhata var\nok\n", ob,
                           sizeof(ob)) == 1 &&
          !strcmp(ob, "hata var\n"), "54D grep");
    {
        const char *fl[3] = {"a.txt", "b.c", "c.txt"};
        char fo[2][64];
        CHECK(textutils64_find(fl, 3, ".txt", fo, 2) == 2 &&
              !strcmp(fo[1], "c.txt"), "54D find");
    }
    CHECK(textutils64_sed_replace("aab", "a", "X", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "XXb"), "54D sed");
    CHECK(textutils64_awk_sum("a 10\nb 20\n", 1) == 30, "54D awk");
    CHECK(textutils64_awk_sum("x -5\ny 7\n", 1) == 2, "54D awk negatif");

    CHECK(sshclient64_banner("SSH-2.0-OpenSSH_9.0", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "2.0"), "54E surum");
    CHECK(sshclient64_banner("TELNET", 0, 0) != 0, "54E red");
    {
        unsigned char pkt[128];
        int rc = sshclient64_packet(20, "AB", 2, pkt, sizeof(pkt));
        CHECK(rc > 0 && pkt[4] == 8 && pkt[5] == 20, "54E paket");
        CHECK(sshclient64_kexinit(pkt, sizeof(pkt)) == 0, "54E kexinit");
        CHECK(sshclient64_kexinit(pkt, 10) != 0, "54E kisa red");
    }

    {
        unsigned char h[20];
        /* SHA-1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d */
        static const unsigned char exp_abc[20] = {
            0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
            0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d
        };
        CHECK(gitclient64_sha1("abc", 3, h) == 0 &&
              !memcmp(h, exp_abc, 20), "54F sha1");
        CHECK(gitclient64_sha1("", 0, h) == 0, "54F bos");
        {
            unsigned char obj[64];
            int rc = gitclient64_object("blob", "abc", 3, obj,
                                        sizeof(obj));
            CHECK(rc == 10 && !memcmp(obj, "blob 3", 6) && obj[6] == 0 &&
                  !memcmp(obj + 7, "abc", 3), "54F nesne");
        }
        {
            char hh[48], nm[48];
            CHECK(gitclient64_ref(
                      "a9993e364706816aba3e25717850c26c9cd0d89d "
                      "refs/heads/ana\n",
                      hh, nm) == 0 && strlen(hh) == 40 &&
                  !strcmp(nm, "refs/heads/ana"), "54F ref");
            CHECK(gitclient64_ref("kisa", 0, 0) != 0, "54F kisa red");
        }
    }

    {
        const char *d1[3] = {"b.o", "c.o", 0};
        char built[8][48];
        CHECK(make64_add_rule("a", d1, "cc -o a") == 0, "54G kural");
        CHECK(make64_add_rule("b.o", 0, "cc -c") == 0, "54G kural2");
        CHECK(make64_set_time("b.o", 10) == 0, "54G zaman");
        CHECK(make64_set_time("c.o", 20) == 0, "54G zaman2");
        CHECK(make64_set_time("a", 5) == 0, "54G zaman3");
        n = make64_build("a", built, 8);
        CHECK(n >= 3 && !strcmp(built[n - 1], "a"), "54G derle sira");
        CHECK(make64_build("yok-hedef", built, 8) >= 1, "54G hayalet");
        CHECK(make64_add_rule("x", (const char *[]){"y", 0}, "r") == 0,
              "54G dongu kur");
        CHECK(make64_add_rule("y", (const char *[]){"x", 0}, "r") == 0,
              "54G dongu kur2");
        CHECK(make64_build("x", built, 8) != 0, "54G dongu red");
    }

    {
        struct python64_obj a, b, c;
        int ok = 0;
        CHECK(python64_int(40, &a) == 0, "54H int");
        CHECK(python64_int(2, &b) == 0, "54H int2");
        CHECK(python64_add(&a, &b, &c) == 0 && c.ival == 42, "54H topla");
        CHECK(python64_str("ab", &a) == 0, "54H str");
        CHECK(python64_str("cd", &b) == 0, "54H str2");
        CHECK(python64_add(&a, &b, &c) == 0 &&
              !strcmp(c.sval, "abcd"), "54H birlestir");
        CHECK(python64_eval_int("2 + 3 * 4 - 1", &ok) == 13 && ok,
              "54H oncelik");
        CHECK(python64_eval_int("(2 + 3) * 4", &ok) == 20 && ok,
              "54H parantez");
        CHECK(python64_eval_int("10 / 3", &ok) == 3 && ok, "54H bolme");
        CHECK(python64_eval_int("10 / 0", &ok) == 0 && !ok,
              "54H sifir red");
        CHECK(python64_eval_int("2 +", &ok) == 0 && !ok, "54H eksik red");
        CHECK(python64_print(&c, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "abcd"), "54H yazdir");
    }

    CHECK(node64_require("fs") == 0, "54I modul");
    CHECK(node64_require("fs") == 0, "54I ayni id");
    CHECK(node64_set_timeout(100, 7, 1000) == 0, "54I zamanlayici");
    CHECK(node64_next_tick(3) == 0, "54I ertelenmis");
    {
        int fired[4];
        int m = node64_tick(1000, fired, 4);
        CHECK(m == 1 && fired[0] == 3, "54I once tick");
        m = node64_tick(1200, fired, 4);
        CHECK(m == 1 && fired[0] == 7, "54I vade");
        CHECK(node64_tick(99999, fired, 4) == 0, "54I bos");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
