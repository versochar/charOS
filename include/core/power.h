#ifndef CHAROS_CORE_POWER_H
#define CHAROS_CORE_POWER_H

/* 31.2: Güç muhasebesi + politika yöneticisi.
 * Timer IRQ her tick'te boşta/meşgul sayar; politika danışımsal
 * (donanım P-state köprüsü takip işi). Aynı dosya iki yerde derlenir.
 */
#include "stdint.h"

#define POWER_PERF     0
#define POWER_BALANCED 1
#define POWER_SAVER    2

void     power_init(void);
int      power_set_policy(int p); /* 0 ok / -1 */
int      power_get_policy(void);
void     power_note_tick(int is_idle);
uint32_t power_idle_ticks(void);
uint32_t power_busy_ticks(void);
uint32_t power_idle_pct(void);    /* 0-100; sayaç yoksa 0 */
int      power_selftest(void);    /* 0 ok */

#endif
