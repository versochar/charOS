/* 44J: rtlprobe64/fwload64/mac11_64/wpa364/scan64/powersave64/apmode64/
 *      mesh64/drvtest64 host testi.
 * Calistirma: make test-wifi64
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

static unsigned char fwblob[128];

static void fw_wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static void build_fw(void) {
    memset(fwblob, 0, sizeof(fwblob));
    fw_wr32(fwblob + 0, 0x46575452UL);
    fw_wr32(fwblob + 4, 7);
    fw_wr32(fwblob + 8, 2);
    fw_wr32(fwblob + 12, 0);
    fw_wr32(fwblob + 16, 0x1000);
    fw_wr32(fwblob + 20, 4);
    fwblob[24] = 1;
    fwblob[25] = 2;
    fwblob[26] = 3;
    fwblob[27] = 4;
    fw_wr32(fwblob + 28, 0x2000);
    fw_wr32(fwblob + 32, 2);
    fwblob[36] = 5;
    fwblob[37] = 6;
}

int main(void) {
    unsigned char mac[6] = {0x02, 0, 0x4C, 0x11, 0x22, 0x33};
    unsigned char out6[6];
    unsigned char frame[64];
    int i;
    u64 sc = 0, el = 0, cf = 0, ptk = 0;
    unsigned char anonce[32];
    unsigned char bssid[6];
    int rssi = 0;
    u32 ver = 0;
    struct fwload64_chunk ch;

    CHECK(rtlprobe64_match(0x10EC, 0xC821) == 0, "44A eslesme");
    CHECK(rtlprobe64_match(0x10EC, 0x1234) != 0, "44A red");
    CHECK(rtlprobe64_rev(0x21) == 0, "44A A-kesim");
    CHECK(rtlprobe64_rev(0xFF) != 0, "44A bilinmeyen red");
    CHECK(rtlprobe64_check_bar(0xF0000000ULL, 0x4000) == 0, "44A bar ok");
    CHECK(rtlprobe64_check_bar(0xF0000001ULL, 0x4000) != 0, "44A io red");
    CHECK(rtlprobe64_check_bar(0x140000000ULL, 0x4000) == 1,
          "44A 64-bit bar notu");

    build_fw();
    CHECK(fwload64_parse(fwblob, 38, &ver) == 0 && ver == 7, "44B ayristir");
    CHECK(fwload64_nchunks() == 2, "44B parca sayisi");
    CHECK(fwload64_chunk(0, &ch) == 0 && ch.addr == 0x1000 && ch.len == 4 &&
          ch.data[0] == 1, "44B parca0");
    CHECK(fwload64_verify() == 0, "44B saglama");
    {
        int st = -1, a, b;
        a = fwload64_next(&st);
        b = fwload64_next(&st);
        CHECK(a == 0 && b == 1 && fwload64_next(&st) < 0, "44B sira");
    }
    CHECK(fwload64_parse("BOZUK", 5, 0) != 0, "44B bozuk red");

    CHECK(mac11_add_iface(MAC11_STA, mac) >= 0, "44C arayuz");
    CHECK(mac11_add_iface(99, mac) != 0, "44C tur red");
    {
        unsigned char key[16];
        for (i = 0; i < 16; i++) key[i] = (unsigned char)i;
        CHECK(mac11_set_key(0, key) == 0, "44C anahtar");
    }
    for (i = 0; i < 64; i++) frame[i] = (unsigned char)i;
    CHECK(mac11_tx(MAC11_AC_BE, frame, sizeof(frame)) == 0, "44C gonder");
    {
        unsigned char back[64];
        CHECK(mac11_rx(MAC11_AC_BE, back, sizeof(back)) == 64 &&
              !memcmp(back, frame, 64), "44C geri-dongu");
    }
    CHECK(mac11_rx(MAC11_AC_BE, frame, sizeof(frame)) == 0, "44C bos");

    CHECK(wpa364_sae_commit("sifre123", &sc, &el) == 0 && sc && el,
          "44D taahhut");
    CHECK(wpa364_state() == 2, "44D durum");
    CHECK(wpa364_sae_confirm(0x1111, 0x2222, &cf) == 0 && cf,
          "44D onay");
    CHECK(wpa364_sae_verify(0x9999, 0x1111, 0x2222) == 0, "44D dogrula");
    for (i = 0; i < 32; i++) anonce[i] = (unsigned char)(i * 3 + 1);
    CHECK(wpa364_4way_msg123(anonce, &ptk) == 0 && ptk, "44D 4-yonlu");
    CHECK(wpa364_4way_msg4(0) == 0, "44D bitir");
    CHECK(wpa364_state() == 5, "44D kurulu");

    {
        unsigned char b1[6] = {0, 1, 2, 3, 4, 5};
        unsigned char b2[6] = {0, 1, 2, 3, 4, 6};
        CHECK(scan64_add(b1, "ev", 6, -60, 1) == 0, "44E ekle");
        CHECK(scan64_add(b2, "ev", 11, -50, 1) == 0, "44E ekle2");
        CHECK(scan64_add(b1, "ev", 6, -55, 1) == 0, "44E guncelle");
        CHECK(scan64_count() == 2, "44E sayac");
        CHECK(scan64_best("ev", bssid, &rssi) == 0 && rssi == -50 &&
              bssid[5] == 6, "44E en guclu");
        CHECK(scan64_best("yok", 0, 0) != 0, "44E yok red");
        CHECK(scan64_roam_needed(-80, -50) == 1, "44E dolasim");
        CHECK(scan64_roam_needed(-60, -50) == 0, "44E yeterli");
        CHECK(scan64_roam_needed(-80, -78) == 0, "44E histerezis");
    }

    CHECK(powersave64_set(POWERSAVE64_DEEP) == 0, "44F derin");
    CHECK(powersave64_mode() == POWERSAVE64_DEEP, "44F kip");
    CHECK(powersave64_beacon(5, 0) == 0, "44F uyu");
    CHECK(powersave64_beacon(0, 1) == 1, "44F dtim uyan");
    CHECK(powersave64_saved_us() > 0, "44F tasarruf");
    CHECK(powersave64_set(99) != 0, "44F kip red");
    CHECK(powersave64_set(POWERSAVE64_ACTIVE) == 0, "44F aktif");

    {
        unsigned char sta[6] = {0x0A, 0, 0, 0, 0, 1};
        unsigned char bc[128];
        CHECK(apmode64_sta_add(sta) == 0, "44G istasyon");
        CHECK(apmode64_clients() == 1, "44G sayac");
        CHECK(apmode64_sta_authorize(sta) == 0, "44G yetkilendir");
        CHECK(apmode64_beacon("charos-ap", 6, bc, sizeof(bc)) > 0 &&
              bc[0] == 0x80, "44G isaret");
        CHECK(apmode64_sta_remove(sta) == 0, "44G kaldir");
        CHECK(apmode64_clients() == 0, "44G bos");
        CHECK(apmode64_sta_authorize(sta) != 0, "44G yok red");
    }

    {
        unsigned char d[6] = {1, 0, 0, 0, 0, 9};
        unsigned char h1[6] = {1, 0, 0, 0, 0, 1};
        unsigned char h2[6] = {1, 0, 0, 0, 0, 2};
        unsigned char hop[6];
        CHECK(mesh64_metric(100, 0) < mesh64_metric(10, 0),
              "44H hiz metrige yansir");
        CHECK(mesh64_metric(100, 10) > mesh64_metric(100, 0),
              "44H hata cezasi");
        CHECK(mesh64_learn(d, h1, 1000) == 0, "44H ogren");
        CHECK(mesh64_learn(d, h2, 500) == 0, "44H iyi yol");
        CHECK(mesh64_route(d, hop) == 0 && hop[5] == 2, "44H yonlendir");
        CHECK(mesh64_route(mac, 0) != 0, "44H yol yok");
    }

    CHECK(drvtest64_regs((0xC821UL << 16) | 0x1) == 0, "44I register");
    CHECK(drvtest64_regs(0x12340000UL) != 0, "44I yanlis id red");
    CHECK(drvtest64_loopback(frame, sizeof(frame)) == 0, "44I dongu");
    CHECK(drvtest64_loopback(0, 10) != 0, "44I null red");
    CHECK(drvtest64_fw_alive(1) == 0, "44I nabiz");
    CHECK(drvtest64_fw_alive(1) == 1, "44I karar-yok");
    for (i = 0; i < 101; i++) drvtest64_fw_alive(1);
    CHECK(drvtest64_fw_alive(1) != 0, "44I takilma");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
