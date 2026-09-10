#ifndef CHAROS_DRIVERS_PCM_H
#define CHAROS_DRIVERS_PCM_H

/* 36.2: PCM halka tamponu (s16 örnekler, DMA besleyici hedefi).
 * Kısmi yazma/okuma + underrun/overrun sayaçları.
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

#define PCM_CAP 512

void     pcm_init(void);
int      pcm_write(const int16_t* s, uint32_t n); /* yazılan sayı / -1 */
int      pcm_read(int16_t* out, uint32_t n);      /* okunan sayı (0 dahil) */
uint32_t pcm_avail(void);
uint32_t pcm_free(void);
uint32_t pcm_underruns(void);
uint32_t pcm_overruns(void);
int      pcm_selftest(void); /* 0 ok */

#endif
