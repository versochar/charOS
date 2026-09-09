#ifndef CHAROS_DRIVERS_CLOCK_H
#define CHAROS_DRIVERS_CLOCK_H

#include <stdint.h>

/* 28A: TSC kalibrasyonu + clock_gettime (ns hassas).
 * TSC (Time Stamp Counter): CPU'da her cycle'da artan 64-bit sayaç.
 * Kalibrasyon: PIT (1193182 Hz) veya HPET üzerinden TSC frekansı hesaplanır. */

/* TSC frekansı (Hz) - kalibrasyondan sonra dolu */
uint64_t tsc_get_freq(void);

/* TSC sayacı (64-bit: hi=EDX, lo=EAX) */
uint64_t tsc_read(void);

/* Kalibrasyon başlat (PIT veya HPET üzerinden). 0=ok, -1=hata */
int clock_init(void);

/* Nanosecond hassas saat (TSC üzerinden). 0=ok, -1=hata */
int clock_gettime(uint64_t* sec, uint32_t* nsec);

/* Saniye cinsinden basit saat */
uint32_t clock_now_sec(void);

/* Self-test: TSC ilerlemesi + kalibrasyon doğrulama */
int clock_selftest(void);

#endif
