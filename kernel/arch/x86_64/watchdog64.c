#include "arch/x86_64/watchdog.h"
static int wdt_on = 0;
static u8 wdt_to = 5;
static u32 wdt_rem = 5;
static int wdt_st = 0; /* 0 OK, 1 WARNING, 2 RESET */
static u32 wdt_ticks = 0;
int watchdog64_init(void) { wdt_on = 1; wdt_to = 5; wdt_rem = 5; wdt_st = 0; wdt_ticks = 0; return 0; }
void watchdog64_timer_step(void) { if (!wdt_on) return; wdt_ticks++; if (wdt_rem == 0) { wdt_rem = 0; } else { wdt_rem--; if (wdt_rem == 0) wdt_st = 2; else if (wdt_rem < (u32)(wdt_to / 2)) wdt_st = 1; else wdt_st = 0; } }
int watchdog64_set_timeout(u8 ticks) { if (!ticks || ticks > 255) return -1; wdt_to = ticks; wdt_rem = ticks; return 0; }
int watchdog64_pet(void) { if (!wdt_on) return -1; wdt_rem = wdt_to; return 0; }
int watchdog64_get_state(int* out) { if (!out) return -1; *out = wdt_st; return 0; }
u32 watchdog64_get_ticks(void) { return wdt_ticks; }
