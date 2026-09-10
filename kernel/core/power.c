/* 31.3: Güç muhasebesi mantığı.
 * Saf sayaç + politika; tick kaynağı timer IRQ'dur (üretici dışarıda).
 * Aynı dosya çekirdekte (freestanding) ve host testinde derlenir.
 */
#include "core/power.h"
#include "core/verify.h"

/* 31.3: derleme-zamanı kanıtı */
STATIC_ASSERT(POWER_PERF < POWER_BALANCED && POWER_BALANCED < POWER_SAVER);

static int policy = POWER_BALANCED;
static uint32_t idle_ticks = 0;
static uint32_t busy_ticks = 0;

void power_init(void) {
    policy = POWER_BALANCED;
    idle_ticks = 0;
    busy_ticks = 0;
}

int power_set_policy(int p) {
    if (REQUIRE(p == POWER_PERF || p == POWER_BALANCED || p == POWER_SAVER,
                0xE201) != 0) return -1;
    policy = p;
    return 0;
}

int power_get_policy(void) {
    return policy;
}

void power_note_tick(int is_idle) {
    if (is_idle) idle_ticks++;
    else busy_ticks++;
}

uint32_t power_idle_ticks(void) {
    return idle_ticks;
}

uint32_t power_busy_ticks(void) {
    return busy_ticks;
}

uint32_t power_idle_pct(void) {
    uint32_t tot = idle_ticks + busy_ticks;
    if (tot == 0) return 0;
    return (idle_ticks * 100u) / tot;
}

int power_selftest(void) {
    power_init();
    if (power_get_policy() != POWER_BALANCED) return -1;
    if (power_idle_pct() != 0) return -2; /* sayaçsız %0 */
    if (power_set_policy(99) == 0) return -3;
    if (power_set_policy(POWER_SAVER) != 0) return -4;
    power_note_tick(1);
    power_note_tick(1);
    power_note_tick(0);
    if (power_idle_ticks() != 2 || power_busy_ticks() != 1) return -5;
    if (power_idle_pct() != 66) return -6; /* 200/3=66 */
    power_init();
    return 0;
}
