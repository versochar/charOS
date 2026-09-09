#ifndef CHAROS_DRIVERS_TIMER_H
#define CHAROS_DRIVERS_TIMER_H

#include <stdint.h>
#include <core/isr.h>

#define PIT_CMD     0x43
#define PIT_DATA0   0x40
#define PIT_DATA1   0x41
#define PIT_DATA2   0x42
#define PIT_FREQ    1193182

void timer_init(uint32_t freq);
void timer_handler(struct registers* regs);
uint32_t timer_get_ticks(void);
void timer_wait(uint32_t ticks); /* hlt-spin, CPU bırakmaz (kısa/kesitli işler) */
void task_sleep(uint32_t ticks); /* BLOCKED uyku, CPU'yu bırakır (polling görevler) */
void timer_set_callback(void (*cb)(void));
void sleep_until(uint32_t until);

#endif