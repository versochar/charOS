#ifndef CHAROS_DRIVERS_SYSMON_H
#define CHAROS_DRIVERS_SYSMON_H

#include <stdint.h>

/* 29F: Sistem takibi - CPU/pil/ağ durum çubuğu (skeleton). */

void sysmon_init(void);
void sysmon_update(void);   /* durum güncelle (CPU yükü, pil, ağ) */
void sysmon_draw(void);     /* durum çubuğunu çiz */
int sysmon_selftest(void);

#endif
