/* 36.5: GERÇEK pcm.c + hdaverb.c testi (aynı dosyalar derlenir).
 * Calistirma: make test-audio36
 */
#include <stdio.h>
#include "drivers/pcm.h"
#include "drivers/hdaverb.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int16_t w[4] = {100, -200, 300, -400};
    int16_t r[4] = {0, 0, 0, 0};

    /* 36.3: sıra korunur */
    pcm_init();
    CHECK(pcm_write(w, 4) == 4, "36.3 yaz");
    CHECK(pcm_avail() == 4 && pcm_free() == PCM_CAP - 4, "36.3 sayaç");
    CHECK(pcm_read(r, 4) == 4 && r[0] == 100 && r[1] == -200 &&
          r[2] == 300 && r[3] == -400, "36.3 sıra");
    CHECK(pcm_avail() == 0, "36.3 bosaldi");

    /* 36.3: underrun/overrun */
    CHECK(pcm_read(r, 1) == 0 && pcm_underruns() == 1, "36.3 underrun");
    {
        int16_t big[PCM_CAP + 5];
        for (int i = 0; i < PCM_CAP + 5; i++) big[i] = (int16_t)i;
        CHECK(pcm_write(big, PCM_CAP + 5) == PCM_CAP, "36.3 kismi");
        CHECK(pcm_overruns() == 1, "36.3 overrun");
    }
    CHECK(pcm_write(0, 1) == -1 && pcm_read(0, 1) == -1, "36.3 null");

    /* 36.3: verb kodlama turu */
    {
        uint32_t c = hdaverb_build(0x11, 0x707, 0x40);
        CHECK(c == 0x01170740u, "36.3 kod");
        CHECK(hdaverb_nid(c) == 0x11 && hdaverb_verb(c) == 0x707 &&
              hdaverb_param(c) == 0x40, "36.3 coz");
        CHECK(hdaverb_build(0x1FFu, 0x1FFFu, 0x1FFu) == 0x0FFFFFFFu,
              "36.3 maske");
    }

    /* 36.3: öztest */
    CHECK(pcm_selftest() == 0, "36.3 selftest");

    /* 36.9: deterministik tekrar */
    {
        pcm_init();
        int16_t a[2] = {7, 8}, b[2] = {0, 0};
        pcm_write(a, 2);
        int n1 = pcm_read(b, 2);
        pcm_init();
        pcm_write(a, 2);
        int n2 = pcm_read(b, 2);
        CHECK(n1 == 2 && n2 == 2 && b[0] == 7 && b[1] == 8, "36.9 deterministik");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
