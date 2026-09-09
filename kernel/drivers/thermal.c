#include <drivers/thermal.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 28F: Termal/güç dengesi - basit skeleton.
 * Gerçek CPU frekans ölçekleme (P-state) CPU modeline göre karmaşık;
 * burası temel kayıt + skeleton. */

static int current_pstate = 0; /* P0 = max */

uint32_t thermal_get_temp(void) {
    /* Gerçek termal sensör (Intel DTS) ileri aşama; skeleton'da sabit */
    return 45; /* varsayılan ~45°C */
}

void thermal_set_pstate(int p) {
    if (p < 0) p = 0;
    if (p > THERMAL_PSTATE_MAX - 1) p = THERMAL_PSTATE_MAX - 1;
    current_pstate = p;
    serial_puts("[28F] P-state="); serial_puthex(p); serial_puts("\n");
}

int thermal_get_pstate(void) { return current_pstate; }

void thermal_dynamic_idle(void) {
    /* Temel: CPU yükü düşükse P-state düşür (skeleton) */
    int temp = (int)thermal_get_temp();
    if (temp > 70) thermal_set_pstate(THERMAL_PSTATE_MAX - 1); /* düşük */
    else thermal_set_pstate(0); /* P0 */
}

int thermal_selftest(void) {
    thermal_set_pstate(0);
    thermal_set_pstate(1);
    thermal_set_pstate(THERMAL_PSTATE_MAX - 1);
    thermal_dynamic_idle();
    serial_puts("[28F] thermal [PASS]\n");
    vga_puts("[28F] thermal [PASS]\n");
    return 0;
}
