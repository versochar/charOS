#ifndef CHAROS_DRIVERS_INPUT_H
#define CHAROS_DRIVERS_INPUT_H

#include <stdint.h>

/* 18A: Input Hub - klavye/fare olayları tek kuyrukta.
 * Üreticiler: keyboard IRQ (canlı), PS/2 mouse (18B+), sentetik test.
 * Tüketiciler: DE (18D+), terminal (18J). */

#define IN_EV_KEY_PRESS   1
#define IN_EV_KEY_RELEASE 2
#define IN_EV_MOUSE_MOVE  3
#define IN_EV_MOUSE_BTN   4

struct input_event {
    uint8_t type;    /* IN_EV_* */
    uint8_t code;    /* tuş: scancode; fare btn: bitmask */
    int16_t x;       /* fare dx (klavyede 0) */
    int16_t y;       /* fare dy (klavyede 0) */
};

void input_init(void);
/* Kuyruk doluysa en eski atılır, 0 dönülür; başarıda 1 */
int input_push(uint8_t type, uint8_t code, int16_t x, int16_t y);
/* Boşsa 0, doluysa 1 + *ev doldurulur */
int input_pop(struct input_event* ev);
int input_pending(void);

/* Klavye sürücüsünden canlı besleme (IRQ bağlamı) */
void input_key(uint8_t scancode, int pressed);

/* Fare besleme (18B+ sürücü için hazır API) */
void input_mouse(int16_t dx, int16_t dy, uint8_t buttons);

/* Boot self-test: sentetik enjekte et + boşalt. 0 PASS. */
int input_selftest(void);

#endif
