#ifndef CHAROS_CORE_PROF_H
#define CHAROS_CORE_PROF_H

/* 26.2: Hafif gecikme profilleyici.
 * Sabit slot tablosu (tahsis yok), enjekte edilebilir tick kaynağı:
 * çekirdekte rdtsc, host'ta sahte sayaç. Aynı dosya iki yerde derlenir.
 */
#include "stdint.h"

#define PROF_MAX_SLOTS 16

typedef uint64_t (*prof_tick_fn)(void);

void     prof_init(void);
void     prof_set_tick(prof_tick_fn fn);
uint64_t prof_tick_rdtsc(void); /* çekirdek varsayılanı; host'ta da çalışır */
int      prof_begin(uint32_t id);          /* 0 ok / -1 hata */
int      prof_end(uint32_t id);            /* 0 ok / -1 hata (açık ölçüm yoksa) */
int      prof_read(uint32_t id, uint32_t* out_count, uint64_t* out_total,
                   uint64_t* out_min, uint64_t* out_max);
int      prof_reset(uint32_t id);

#endif
