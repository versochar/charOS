#ifndef CHAROS_DRIVERS_THERMAL_H
#define CHAROS_DRIVERS_THERMAL_H

#include <stdint.h>

/* 28F: Termal/güç dengesi - CPU P-state, dynamic idle, basit. */

/* P-state değerleri (temel: P0 = max, P1-Pn = düşük) */
#define THERMAL_PSTATE_MAX 4

/* Termal durum: sıcaklık (temel, sabit değer - gerçek sensör ileri aşama) */
uint32_t thermal_get_temp(void);  /* 0-100 arası (varsayılan) */

/* CPU frekans ölçekleme (P-state): düşük frekansa geçiş */
void thermal_set_pstate(int p);
int thermal_get_pstate(void);

/* Dynamic idle: CPU kullanımına göre idle döngüsü */
void thermal_dynamic_idle(void);

/* Self-test: P-state değişimi + idle kontrolü */
int thermal_selftest(void);

/* 32.2: regülatör (histerezisli) + enjekte sensör + kritik sayacı */
void thermal_init(void);
void thermal_set_sensor(uint32_t (*fn)(void)); /* NULL: model tahminci */
int thermal_tick(uint32_t temp);   /* politika adımı, o anki pstate döner */
uint32_t thermal_last_temp(void);
uint32_t thermal_trips(void);      /* kritik eşik aşım sayacı */

#endif
