/* 48J: acpiparse64/s364/s464/cpufreq64/cpuidle64/powerbtn64/battery64/
 *      thermal64/wol64 host testi.
 * Calistirma: make test-power64
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

static int dev_ok(void) { return 0; }
static int dev_bad(void) { return -1; }

static unsigned char xsdt[64];
static unsigned char facp[128];

static void wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

int main(void) {
    struct acpiparse64_fadt f;
    u8 a = 0, b = 0;
    int i;
    unsigned char page[4096];

    memset(xsdt, 0, sizeof(xsdt));
    memcpy(xsdt, "XSDT", 4);
    xsdt[4] = 36 + 8;
    for (i = 0; i < 8; i++)
        xsdt[36 + i] = (unsigned char)(((u64)facp >> (i * 8)) & 0xFF);
    memset(facp, 0, sizeof(facp));
    memcpy(facp, "FACP", 4);
    wr32(facp + 40, 0x1000);
    wr32(facp + 48, 0xB2);
    wr32(facp + 64, 0x400);
    CHECK(acpiparse64_fadt(xsdt, sizeof(xsdt), &f) == 0 &&
          f.dsdt == 0x1000 && f.pm1a_cnt == 0x400, "48A fadt");
    CHECK(acpiparse64_sleep(&f, &a, &b) == 0 && a == 5 && b == 5,
          "48A uyku bilgisi");
    facp[64] = 0;
    facp[65] = 0;
    facp[66] = 0;
    facp[67] = 0;
    CHECK(acpiparse64_fadt(xsdt, sizeof(xsdt), &f) != 0,
          "48A pm1siz red");
    wr32(facp + 64, 0x400);

    CHECK(s364_register("disk", dev_ok, dev_ok) == 0, "48B kayit");
    CHECK(s364_register("net", dev_bad, dev_ok) == 0, "48B kayit2");
    CHECK(s364_suspend(5, 5) != 0, "48B hatali cihaz durdurur");
    CHECK(s364_state() == 0, "48B durum korundu");
    CHECK(s364_resume() != 0, "48B askisiz uyanma red");

    for (i = 0; i < 4096; i++) page[i] = (unsigned char)(i & 0xFF);
    CHECK(s464_begin(2) == 0, "48C baslat");
    CHECK(s464_write_page(0, page) == 0, "48C yaz0");
    CHECK(s464_write_page(1, page) == 0, "48C yaz1");
    CHECK(s464_commit() == 0, "48C isle");
    CHECK(s464_restore_verify() == 0, "48C dogrula");
    CHECK(s464_begin(2) == 0, "48C yeniden");
    CHECK(s464_write_page(0, page) == 0, "48C parcali");
    CHECK(s464_commit() != 0, "48C eksik red");

    CHECK(cpufreq64_add_state(800, 5000) == 0, "48D durum");
    CHECK(cpufreq64_add_state(2400, 15000) == 0, "48D durum2");
    CHECK(cpufreq64_add_state(1600, 9000) == 0, "48D sira");
    CHECK(cpufreq64_governor(95) == 2400, "48D yuksek yuk");
    CHECK(cpufreq64_governor(5) == 800, "48D dusuk yuk");
    CHECK(cpufreq64_governor(50) == 1600, "48D orta yuk");
    CHECK(cpufreq64_set(1600) == 0 && cpufreq64_get() == 1600,
          "48D kur");
    CHECK(cpufreq64_set(9999) != 0, "48D yok red");

    CHECK(cpuidle64_add_state("C1", 2, 1000) >= 0, "48E c1");
    CHECK(cpuidle64_add_state("C3", 50, 200) >= 0, "48E c3");
    CHECK(cpuidle64_add_state("C6", 200, 50) >= 0, "48E c6");
    {
        int c1 = 0, c3 = 1, c6 = 2;
        CHECK(cpuidle64_pick(10) == c1, "48E kisa C1");
        CHECK(cpuidle64_pick(500) == c6, "48E uzun C6");
        CHECK(cpuidle64_pick(1) == -1, "48E cok kisa red");
        CHECK(cpuidle64_residency(c3, 1000) == 0, "48E kalis");
        (void)c6;
    }

    CHECK(powerbtn64_event(500) == 1, "48F kisa");
    CHECK(powerbtn64_event(5000) == 2, "48F uzun");
    CHECK(powerbtn64_read() == 1, "48F oku1");
    CHECK(powerbtn64_read() == 2, "48F oku2");
    CHECK(powerbtn64_read() == 0, "48F bos");
    CHECK(powerbtn64_action(1) == 1, "48F ilke aski");
    CHECK(powerbtn64_action(2) == 2, "48F ilke kapat");

    CHECK(battery64_update(50000, 48000, 24000, 11000, 0) == 0,
          "48G guncelle");
    battery64_set_rate(12000);
    CHECK(battery64_pct() == 50, "48G yuzde");
    CHECK(battery64_state() == 2, "48G desarj");
    CHECK(battery64_minutes() == 120, "48G sure");
    CHECK(battery64_update(50000, 48000, 48000, 12600, 1) == 0,
          "48G dolu");
    CHECK(battery64_state() == 3, "48G dolu durum");

    CHECK(thermal64_set_temp(0, 60) == 0, "48H sicaklik");
    CHECK(thermal64_check(0) == 0, "48H normal");
    CHECK(thermal64_set_temp(0, 90) == 0, "48H isinma");
    CHECK(thermal64_check(0) == 1, "48H pasif");
    CHECK(thermal64_cooling(0, 5) == 0, "48H sogutma");
    CHECK(thermal64_set_temp(0, 105) == 0, "48H kritik");
    CHECK(thermal64_check(0) == 2, "48H kritik durum");
    CHECK(thermal64_check(99) != 0, "48H bolge red");

    {
        unsigned char mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        unsigned char pkt[110];
        unsigned char mask[6] = {0xFF, 0xFF, 0xFF, 0, 0, 0};
        unsigned char pat[6] = {0xAA, 0xBB, 0xCC, 0, 0, 0};
        CHECK(wol64_magic(mac, pkt) == 102 && pkt[0] == 0xFF &&
              pkt[6] == 0xAA && pkt[101] == 0xFF, "48I sihirli");
        CHECK(wol64_match(pkt, sizeof(pkt)) == 1, "48I eslesme");
        pkt[50] = 0;
        CHECK(wol64_match(pkt, sizeof(pkt)) == 0, "48I bozuk red");
        CHECK(wol64_add_pattern(0, mask, pat, 6) == 0, "48I desen");
        {
            unsigned char fr[16] = {0xAA, 0xBB, 0xCC, 1, 2, 3};
            CHECK(wol64_match(fr, sizeof(fr)) == 1, "48I desen eslesme");
            fr[0] = 0;
            CHECK(wol64_match(fr, sizeof(fr)) == 0, "48I desen red");
        }
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
