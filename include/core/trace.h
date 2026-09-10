#ifndef CHAROS_CORE_TRACE_H
#define CHAROS_CORE_TRACE_H

/* 25.2: Seviyeli izleme (trace) katmanı.
 * syslog ham metinken trace seviye + etiket + eşik filtresi + sayaç sunar.
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

#define TRACE_DEBUG 0
#define TRACE_INFO  1
#define TRACE_WARN  2
#define TRACE_ERROR 3
#define TRACE_FATAL 4
#define TRACE_LEVELS 5

typedef void (*trace_backend_fn)(uint32_t level, const char* tag, const char* msg);

void     trace_init(void);
void     trace_set_backend(trace_backend_fn fn);
void     trace_set_level(uint32_t level); /* altındaki seviyeler düşer */
int      trace_event(uint32_t level, const char* tag, const char* msg);
uint32_t trace_count(uint32_t level);     /* iletilen olay sayacı */

#endif
