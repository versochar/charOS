/* 35.3: Taşma-güvenli kırpma matematiği.
 * Tüm ara hesaplar 64-bit: INT_MIN/INT_MAX girdilerinde UB yok.
 * Aynı dosya çekirdekte (freestanding) ve host testinde derlenir.
 */
#include "drivers/raster.h"
#include "core/verify.h"

/* 35.3: derleme-zamanı kanıtı */
STATIC_ASSERT(sizeof(int64_t) == 8);

int raster_clip_rect(int x, int y, int w, int h, int bw, int bh,
                     int* ox, int* oy, int* ow, int* oh) {
    int64_t x0, y0, x1, y1;
    if (REQUIRE(ox != 0 && oy != 0 && ow != 0 && oh != 0, 0xE601) != 0)
        return 0;
    if (REQUIRE(w > 0 && h > 0 && bw > 0 && bh > 0, 0xE602) != 0) return 0;
    x0 = x > 0 ? (int64_t)x : 0;
    y0 = y > 0 ? (int64_t)y : 0;
    x1 = (int64_t)x + (int64_t)w;
    y1 = (int64_t)y + (int64_t)h;
    if (x1 > bw) x1 = bw;
    if (y1 > bh) y1 = bh;
    if (x1 <= x0 || y1 <= y0) return 0;
    *ox = (int)x0;
    *oy = (int)y0;
    *ow = (int)(x1 - x0);
    *oh = (int)(y1 - y0);
    return 1;
}

int raster_point_in(int x, int y, int bw, int bh) {
    if (bw <= 0 || bh <= 0) return 0;
    return x >= 0 && y >= 0 && (int64_t)x < (int64_t)bw &&
           (int64_t)y < (int64_t)bh;
}

int raster_selftest(void) {
    int ox = -1, oy = -1, ow = -1, oh = -1;
    /* tam iç */
    if (!raster_clip_rect(10, 10, 100, 60, 1920, 1080, &ox, &oy, &ow, &oh))
        return -1;
    if (ox != 10 || oy != 10 || ow != 100 || oh != 60) return -2;
    /* negatif taşma */
    if (!raster_clip_rect(-5, -5, 10, 10, 100, 100, &ox, &oy, &ow, &oh))
        return -3;
    if (ox != 0 || oy != 0 || ow != 5 || oh != 5) return -4;
    /* sağ-alt taşma */
    if (!raster_clip_rect(90, 90, 50, 50, 100, 100, &ox, &oy, &ow, &oh))
        return -5;
    if (ox != 90 || oy != 90 || ow != 10 || oh != 10) return -6;
    /* tamamen dış + boş + INT_MAX (taşma UB'siz red) */
    if (raster_clip_rect(200, 200, 10, 10, 100, 100, &ox, &oy, &ow, &oh))
        return -7;
    if (raster_clip_rect(0, 0, 0, 10, 100, 100, &ox, &oy, &ow, &oh))
        return -8;
    if (raster_clip_rect(0, 0, 2147483647, 2147483647, 100, 100,
                         &ox, &oy, &ow, &oh) != 1)
        return -9;
    if (ow != 100 || oh != 100) return -10;
    if (!raster_point_in(0, 0, 100, 100)) return -11;
    if (raster_point_in(-1, 0, 100, 100)) return -12;
    if (raster_point_in(100, 0, 100, 100)) return -13;
    return 0;
}
