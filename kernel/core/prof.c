/* 26.3: Hafif profilleyici mantığı.
 * Saf sayaç + min/max/toplam; tick kaynağı enjekte edilir.
 * Aynı dosya çekirdekte (freestanding) ve host testinde derlenir.
 */
#include "core/prof.h"
#include "core/verify.h"

/* 26.3: derleme-zamanı kanıtı */
STATIC_ASSERT(PROF_MAX_SLOTS > 0 && PROF_MAX_SLOTS <= 64);

typedef struct {
    uint64_t start;
    uint64_t total;
    uint64_t min;
    uint64_t max;
    uint32_t count;
    int open;
} prof_slot_t;

static prof_slot_t slots[PROF_MAX_SLOTS];
static prof_tick_fn tickfn = 0;

void prof_init(void) {
    tickfn = 0;
    for (int i = 0; i < PROF_MAX_SLOTS; i++) {
        slots[i].start = 0;
        slots[i].total = 0;
        slots[i].min = 0;
        slots[i].max = 0;
        slots[i].count = 0;
        slots[i].open = 0;
    }
}

void prof_set_tick(prof_tick_fn fn) {
    tickfn = fn;
}

uint64_t prof_tick_rdtsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

int prof_begin(uint32_t id) {
    if (REQUIRE(id < PROF_MAX_SLOTS, 0xD101) != 0) return -1;
    if (REQUIRE(tickfn != 0, 0xD102) != 0) return -1;
    if (REQUIRE(!slots[id].open, 0xD103) != 0) return -1; /* iç içe ölçüm yok */
    slots[id].start = tickfn();
    slots[id].open = 1;
    return 0;
}

int prof_end(uint32_t id) {
    uint64_t now, dt;
    if (REQUIRE(id < PROF_MAX_SLOTS, 0xD104) != 0) return -1;
    if (REQUIRE(tickfn != 0, 0xD105) != 0) return -1;
    if (REQUIRE(slots[id].open, 0xD106) != 0) return -1;
    now = tickfn();
    dt = now - slots[id].start; /* u64 sarmalı güvenli */
    slots[id].open = 0;
    if (slots[id].count == 0 || dt < slots[id].min) slots[id].min = dt;
    if (dt > slots[id].max) slots[id].max = dt;
    slots[id].total += dt;
    slots[id].count++;
    return 0;
}

int prof_read(uint32_t id, uint32_t* out_count, uint64_t* out_total,
              uint64_t* out_min, uint64_t* out_max) {
    if (REQUIRE(id < PROF_MAX_SLOTS, 0xD107) != 0) return -1;
    if (REQUIRE(out_count != 0 && out_total != 0 && out_min != 0 && out_max != 0,
                0xD108) != 0) return -1;
    *out_count = slots[id].count;
    *out_total = slots[id].total;
    *out_min = slots[id].min;
    *out_max = slots[id].max;
    return 0;
}

int prof_reset(uint32_t id) {
    if (REQUIRE(id < PROF_MAX_SLOTS, 0xD109) != 0) return -1;
    slots[id].total = 0;
    slots[id].min = 0;
    slots[id].max = 0;
    slots[id].count = 0;
    slots[id].open = 0;
    return 0;
}
