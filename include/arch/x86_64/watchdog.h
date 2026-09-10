#ifndef WATCHDOG_H
#define WATCHDOG_H
#include "arch/x86_64/longmode.h"
int watchdog64_init(void);
int watchdog64_set_timeout(u8 ticks);
int watchdog64_pet(void);
int watchdog64_get_state(int* out_state);
u32 watchdog64_get_ticks(void);
void watchdog64_timer_step(void);
#endif
