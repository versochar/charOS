/* 35.5: GERÇEK kernel/drivers/raster.c testi (aynı dosya derlenir).
 * Düşman girdiler dahil (negatif, INT_MIN/MAX, taşma).
 * Calistirma: make test-raster35
 */
#include <stdio.h>
#include <limits.h>
#include "drivers/raster.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int ox = -1, oy = -1, ow = -1, oh = -1;

    /* 35.3: temel kırpma */
    CHECK(raster_clip_rect(10, 10, 100, 60, 1920, 1080, &ox, &oy, &ow, &oh) &&
          ox == 10 && oy == 10 && ow == 100 && oh == 60, "35.3 ic");
    CHECK(raster_clip_rect(-5, -5, 10, 10, 100, 100, &ox, &oy, &ow, &oh) &&
          ox == 0 && oy == 0 && ow == 5 && oh == 5, "35.3 negatif");
    CHECK(!raster_clip_rect(200, 200, 10, 10, 100, 100, &ox, &oy, &ow, &oh),
          "35.3 dis");
    CHECK(!raster_clip_rect(0, 0, 10, 10, 100, 100, NULL, &oy, &ow, &oh),
          "35.3 null");

    /* 35.3: düşman girdiler (taşma UB'siz) */
    CHECK(raster_clip_rect(0, 0, INT_MAX, INT_MAX, 100, 100, &ox, &oy, &ow, &oh) &&
          ow == 100 && oh == 100, "35.3 intmax");
    CHECK(!raster_clip_rect(INT_MIN, INT_MIN, 10, 10, 100, 100, &ox, &oy, &ow, &oh),
          "35.3 intmin");
    CHECK(!raster_clip_rect(0, 0, -5, 10, 100, 100, &ox, &oy, &ow, &oh),
          "35.3 negatif-boyut");
    CHECK(!raster_clip_rect(0, 0, 10, 10, 0, 100, &ox, &oy, &ow, &oh),
          "35.3 bos-hedef");

    /* 35.3: nokta */
    CHECK(raster_point_in(0, 0, 100, 100), "35.3 nokta-ic");
    CHECK(!raster_point_in(100, 100, 100, 100), "35.3 nokta-sinir");
    CHECK(!raster_point_in(-1, -1, 100, 100), "35.3 nokta-neg");

    /* 35.3: öztest */
    CHECK(raster_selftest() == 0, "35.3 selftest");

    /* 35.9: deterministik tekrar */
    {
        int a1 = -1, b1 = -1, c1 = -1, d1 = -1, a2 = -1, b2 = -1, c2 = -1, d2 = -1;
        int r1 = raster_clip_rect(-3, 5, 50, 50, 64, 64, &a1, &b1, &c1, &d1);
        int r2 = raster_clip_rect(-3, 5, 50, 50, 64, 64, &a2, &b2, &c2, &d2);
        CHECK(r1 && r2 && a1 == a2 && b1 == b2 && c1 == c2 && d1 == d2 &&
              a1 == 0 && c1 == 47, "35.9 deterministik");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
