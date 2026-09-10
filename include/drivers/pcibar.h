#ifndef CHAROS_DRIVERS_PCIBAR_H
#define CHAROS_DRIVERS_PCIBAR_H

/* 34.2: BAR kodçözüm + boyut yoklama (saf mantık).
 * 64-bit BAR, prefetchable ve IO maskeleri burada tekleşir.
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

/* lo/hi: BAR okumaları. base/is_io/is64/prefetch doldurulur. 0 ok / -1. */
int pcibar_decode(uint32_t lo, uint32_t hi, uint64_t* base,
                  int* is_io, int* is64, int* prefetch);
/* Yoklama değerinden boyut (0: uygulanmamış BAR). */
uint64_t pcibar_size32(uint32_t probe);
uint64_t pcibar_size64(uint32_t lo_probe, uint32_t hi_probe);
uint64_t pcibar_sizeio(uint32_t probe);
int pcibar_selftest(void); /* 0 ok */

#endif
