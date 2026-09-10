/* 25.3: Seviyeli izleme mantığı.
 * Saf filtre + sayaç; arka uç kanca ile enjekte edilir (çekirdekte syslog).
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "core/trace.h"
#include "core/verify.h"

/* 25.3: derleme-zamanı kanıtları (sıralama + sayaç boyutu) */
STATIC_ASSERT(TRACE_DEBUG < TRACE_INFO && TRACE_INFO < TRACE_WARN &&
              TRACE_WARN < TRACE_ERROR && TRACE_ERROR < TRACE_FATAL);
static uint32_t tcounts[TRACE_LEVELS];
static trace_backend_fn tbackend = 0;
static uint32_t tlevel = TRACE_INFO;

void trace_init(void) {
    tbackend = 0;
    tlevel = TRACE_INFO;
    for (int i = 0; i < TRACE_LEVELS; i++) tcounts[i] = 0;
}

void trace_set_backend(trace_backend_fn fn) {
    tbackend = fn;
}

void trace_set_level(uint32_t level) {
    if (level >= TRACE_LEVELS) return;
    tlevel = level;
}

int trace_event(uint32_t level, const char* tag, const char* msg) {
    if (!tag || !msg || level >= TRACE_LEVELS) return -1;
    if (level < tlevel) return -1;  /* eşik altı düşer, sayılmaz */
    if (!tbackend) return -1;       /* gidecek yer yok */
    tbackend(level, tag, msg);
    tcounts[level]++;
    return 0;
}

uint32_t trace_count(uint32_t level) {
    if (level >= TRACE_LEVELS) return 0;
    return tcounts[level];
}
