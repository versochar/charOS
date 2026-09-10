#ifndef CHAROS_DRIVERS_RASTER_H
#define CHAROS_DRIVERS_RASTER_H

/* 35.2: Taşma-güvenli kırpma çekirdeği (saf matematik).
 * Mevcut çizim kodu int toplamada teorik taşma UB'si taşır;
 * burada ara hesap 64-bit yapılır. Aynı dosya iki yerde derlenir.
 */
#include "stdint.h"

/* Dikdörtgeni [0,bw)x[0,bh) ile kırp. 1=görünür (out dolar), 0=boş/geçersiz. */
int raster_clip_rect(int x, int y, int w, int h, int bw, int bh,
                     int* ox, int* oy, int* ow, int* oh);
/* Nokta sınırda mı (1/0). */
int raster_point_in(int x, int y, int bw, int bh);
int raster_selftest(void); /* 0 ok */

#endif
