/* 31.5: GERÇEK kernel/core/power.c testi (aynı dosya derlenir).
 * Calistirma: make test-power31
 */
#include <stdio.h>
#include "core/power.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    /* 31.3: politika */
    power_init();
    CHECK(power_get_policy() == POWER_BALANCED, "31.3 varsayilan");
    CHECK(power_set_policy(POWER_SAVER) == 0 &&
          power_get_policy() == POWER_SAVER, "31.3 politika");
    CHECK(power_set_policy(99) == -1 &&
          power_get_policy() == POWER_SAVER, "31.3 aralik korunur");

    /* 31.3: sayaç + yüzde */
    power_init();
    CHECK(power_idle_pct() == 0, "31.3 bos-%0");
    power_note_tick(1);
    power_note_tick(1);
    power_note_tick(0);
    CHECK(power_idle_ticks() == 2 && power_busy_ticks() == 1, "31.3 sayac");
    CHECK(power_idle_pct() == 66, "31.3 yuzde");

    /* 31.3: öztest */
    CHECK(power_selftest() == 0, "31.3 selftest");

    /* 31.9: deterministik tekrar */
    power_init();
    for (int i = 0; i < 10; i++) power_note_tick(i % 2);
    CHECK(power_idle_ticks() == 5 && power_busy_ticks() == 5, "31.9 yari");
    CHECK(power_idle_pct() == 50, "31.9 yuzde-50");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
