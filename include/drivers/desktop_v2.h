#ifndef CHAROS_DRIVERS_DESKTOP_V2_H
#define CHAROS_DRIVERS_DESKTOP_V2_H

#include <stdint.h>

/* 29A: 1920x1080 masaüstü v2 (ölçekleme, HiDPI-lite).
 * Gerçek masaüstü yöneticisi (pencere, kompozitör) ileri aşama;
 * burası skeleton + çözünürlük tanımlama + HiDPI temel. */

/* Masaüstü çözünürlük modları */
#define DESK_V2_WIDTH  1920
#define DESK_V2_HEIGHT 1080
#define DESK_V2_BPP    32

/* HiDPI ölçek faktörü (1.0 = normal, 2.0 = 2x HiDPI-lite) */
#define DESK_V2_HIDPI_SCALE 2

/* Masaüstü başlatma (GOP/VESA mod ayarlama + ölçekli yüzey) */
int desktop_v2_init(void);

/* Çözünürlük bilgisi */
void desktop_v2_get_res(uint32_t* w, uint32_t* h, uint32_t* scale);

/* Self-test: 1920x1080 tanımlama + HiDPI ölçek kontrolü */
int desktop_v2_selftest(void);

#endif
