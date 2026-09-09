/* 51J: musl64/pthread64/dnsresolv64/stdio64/locale64/tz64/iconv64/
 *      regex64/math64 host testi.
 * Calistirma: make test-libc64
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
    char buf[128];
    char ob[64];

    {
        char a[16] = "0123456789abcdef";
        char b[16];
        musl64_memcpy(b, a, 16);
        CHECK(!memcmp(a, b, 16), "51A memcpy");
        musl64_memmove(a + 2, a, 8);
        CHECK(!memcmp(a + 2, "01234567", 8), "51A memmove ortusme");
        musl64_memset(b, 0x5A, 16);
        CHECK(b[0] == 0x5A && b[15] == 0x5A, "51A memset");
        CHECK(musl64_strlen("merhaba") == 7, "51A strlen");
        CHECK(musl64_strcmp("abc", "abd") < 0, "51A strcmp");
        CHECK(musl64_version(buf, sizeof(buf)) == 0, "51A surum");
    }

    {
        int id = -1, rv = -1, m;
        CHECK(pthread64_create(&id) == 0 && id >= 0, "51B olustur");
        CHECK(pthread64_self() == 0, "51B self");
        m = pthread64_mutex_init();
        CHECK(m >= 0, "51B mutex");
        CHECK(pthread64_mutex_lock(m) == 0, "51B kilit");
        CHECK(pthread64_mutex_lock(m) == 0, "51B ozyinelemeli");
        CHECK(pthread64_mutex_unlock(m) == 0, "51B birak1");
        CHECK(pthread64_mutex_unlock(m) == 0, "51B birak2");
        CHECK(pthread64_mutex_unlock(m) != 0, "51B asiri red");
        CHECK(pthread64_cond_wait(0, m) == 0, "51B bekle");
        CHECK(pthread64_cond_signal(0) == 0, "51B sinyal");
        CHECK(pthread64_cond_broadcast(0) == 0, "51B yayin");
        CHECK(pthread64_join(id, &rv) == 0, "51B birles");
        CHECK(pthread64_detach(id) != 0, "51B bitmis red");
    }

    CHECK(dnsresolv64_nameserver("8.8.8.8") == 0, "51C sunucu");
    CHECK(dnsresolv64_search("yerel") == 0, "51C arama");
    CHECK(dnsresolv64_add_host("yazici", "192.168.1.20") == 0, "51C host");
    CHECK(dnsresolv64_lookup("yazici", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "192.168.1.20"), "51C hosts");
    CHECK(dnsresolv64_lookup("YAZICI", ob, sizeof(ob)) == 0,
          "51C duyarsiz");
    {
        dns64_cache_add("a.b", 0x01020304UL);
        CHECK(dnsresolv64_lookup("a.b", ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "1.2.3.4"), "51C onbellek");
    }
    CHECK(dnsresolv64_lookup("bilinmeyen", ob, sizeof(ob)) != 0,
          "51C yok red");

    {
        int f = stdio64_open(STDIO64_LINE);
        CHECK(f >= 0, "51D ac");
        CHECK(stdio64_write(f, "satir1\nkismi", 12) == 12, "51D yaz");
        CHECK(stdio64_seek(f, 0) == 0, "51D basa sar");
        {
            char r[16];
            memset(r, 0, sizeof(r));
            /* Satir kipi yikadi: "satir1\n" gitti, "kismi" kaldi */
            CHECK(stdio64_read(f, r, sizeof(r)) == 5 &&
                  !strcmp(r, "kismi"), "51D satir tampon");
        }
        CHECK(stdio64_close(f) == 0, "51D kapat");
        CHECK(stdio64_open(99) != 0, "51D kip red");
    }

    CHECK(locale64_set("tr_TR") == 0, "51E sec");
    {
        char dec[8], tho[8], cur[8];
        CHECK(locale64_conv(dec, tho, cur, sizeof(dec)) == 0 &&
              !strcmp(dec, ",") && !strcmp(tho, ".") &&
              !strcmp(cur, "TL"), "51E donusum");
        CHECK(locale64_format_num(1234567, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "1.234.567"), "51E gruplama");
        CHECK(locale64_format_num(-42, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "-42"), "51E negatif");
        CHECK(locale64_set("xx_YY") != 0, "51E yok red");
    }

    CHECK(tz64_add_rule("TR", 180, 180, 3, 5, 10, 5) == 0, "51F kural");
    CHECK(tz64_offset("TR", 2026, 7, 15, 12) == 180, "51F yaz");
    CHECK(tz64_offset("TR", 2026, 1, 15, 12) == 180, "51F kis");
    CHECK(tz64_utc_to_local("TR", 1000000, 2026, 7, 1, 0) ==
              1000000 + 180 * 60, "51F ceviri");
    CHECK(tz64_add_rule("X", 0, 0, 13, 1, 1, 1) != 0, "51F ay red");

    {
        int cd = iconv64_open("UTF-8", "UTF-16");
        const unsigned char in16[4] = {0x41, 0x00, 0xDF, 0x00};
        const unsigned char *ip = in16;
        u64 il = 4;
        unsigned char o8[8];
        unsigned char *op = o8;
        u64 ol = sizeof(o8);
        CHECK(cd >= 0, "51G ac");
        CHECK(iconv64_convert(cd, &ip, &il, &op, &ol) == 0 && il == 0,
              "51G donustur");
        CHECK(o8[0] == 'A' && o8[1] == 0xC3 && o8[2] == 0x9F,
              "51G icerik");
        CHECK(iconv64_close(cd) == 0, "51G kapat");
        {
            int cd2 = iconv64_open("UTF-8", "UTF-8");
            const unsigned char bad[2] = {0xC3, 0x28};
            const unsigned char *bp = bad;
            u64 bl = 2;
            unsigned char bo[8];
            unsigned char *bop = bo;
            u64 bol = sizeof(bo);
            CHECK(iconv64_convert(cd2, &bp, &bl, &bop, &bol) != 0,
                  "51G bozuk red");
            iconv64_close(cd2);
        }
        CHECK(iconv64_open("UTF-8", "EBCDIC") != 0, "51G kod red");
    }

    {
        int r = regex64_compile("a[0-9]+b");
        int st = -1;
        CHECK(r >= 0, "51H derle");
        CHECK(regex64_match(r, "a123b") == 0, "51H tam");
        CHECK(regex64_match(r, "a123c") != 0, "51H red");
        CHECK(regex64_match(r, "xa123b") != 0, "51H bastan red");
        CHECK(regex64_search(r, "xxa7bYY", &st) == 0 && st == 2,
              "51H ara");
        {
            int r2 = regex64_compile("^ab*c$");
            CHECK(regex64_match(r2, "abbbc") == 0, "51H capali");
            CHECK(regex64_match(r2, "xabbbc") != 0, "51H capa red");
        }
        {
            int r3 = regex64_compile("colou?r");
            CHECK(regex64_match(r3, "color") == 0, "51H opsiyonel");
            CHECK(regex64_match(r3, "colour") == 0, "51H opsiyonel2");
            CHECK(regex64_match(r3, "colouur") != 0, "51H fazla red");
        }
        {
            int r4 = regex64_compile("[^0-9]+");
            CHECK(regex64_match(r4, "abc") == 0, "51H degilleme");
            CHECK(regex64_match(r4, "a1") != 0, "51H degilleme red");
        }
        CHECK(regex64_match(99, "x") != 0, "51H handle red");
    }

    CHECK(math64_isqrt(81) == 9, "51I kok");
    CHECK(math64_isqrt(82) == 9, "51I kok taban");
    CHECK(math64_icbrt(1000) == 10, "51I kup");
    CHECK(math64_ilog2(1024) == 10, "51I log2");
    CHECK(math64_ilog2(0) < 0, "51I sifir red");
    CHECK(math64_pow_u64(2, 10) == 1024, "51I us");
    CHECK(math64_gcd(48, 18) == 6, "51I ebob");
    {
        int s0 = math64_sin_q16(0);
        int s90 = math64_sin_q16(90);
        int s180 = math64_sin_q16(180);
        int s270 = math64_sin_q16(270);
        int c0 = math64_cos_q16(0);
        CHECK(s0 == 0, "51I sin0");
        CHECK(s90 > 60000, "51I sin90");
        CHECK(s180 == 0, "51I sin180");
        CHECK(s270 < -60000, "51I sin270");
        CHECK(c0 > 60000, "51I cos0");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
