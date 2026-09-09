#include <drivers/input.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 18A: Input Hub - 128 girişli halka kuyruk */

#define IN_QSIZE 128

static struct input_event in_q[IN_QSIZE];
static volatile uint16_t in_head = 0;
static volatile uint16_t in_tail = 0;
static spinlock_t in_lock = SPINLOCK_INIT;
static volatile uint32_t in_dropped = 0;

void input_init(void) {
    in_head = in_tail = 0;
    in_dropped = 0;
    serial_puts("[18A] input hub hazır\n");
}

/* Fare akıcılığı: biriken hareketler doygun toplamla birleşir (±4096).
 * Hızlı harekette kuyruk şişmez, olay başına 1 flip yeter. */
#define IN_COALESCE_MAX 4096
int input_push(uint8_t type, uint8_t code, int16_t x, int16_t y) {
    uint32_t flags;
    spin_lock_irqsave(&in_lock, &flags);
    if (type == IN_EV_MOUSE_MOVE && in_head != in_tail) {
        uint16_t last = (in_head + IN_QSIZE - 1) % IN_QSIZE;
        if (in_q[last].type == IN_EV_MOUSE_MOVE) {
            int nx = (int)in_q[last].x + (int)x;
            int ny = (int)in_q[last].y + (int)y;
            if (nx > IN_COALESCE_MAX) nx = IN_COALESCE_MAX;
            if (nx < -IN_COALESCE_MAX) nx = -IN_COALESCE_MAX;
            if (ny > IN_COALESCE_MAX) ny = IN_COALESCE_MAX;
            if (ny < -IN_COALESCE_MAX) ny = -IN_COALESCE_MAX;
            in_q[last].x = (int16_t)nx;
            in_q[last].y = (int16_t)ny;
            in_q[last].code |= code;
            spin_unlock_irqrestore(&in_lock, flags);
            return 1;
        }
    }
    uint16_t next = (in_head + 1) % IN_QSIZE;
    if (next == in_tail) {
        in_dropped++;
        spin_unlock_irqrestore(&in_lock, flags);
        return 0; /* dolu: en eski korunur, yeni atılır */
    }
    in_q[in_head].type = type;
    in_q[in_head].code = code;
    in_q[in_head].x = x;
    in_q[in_head].y = y;
    in_head = next;
    spin_unlock_irqrestore(&in_lock, flags);
    return 1;
}

int input_pop(struct input_event* ev) {
    uint32_t flags;
    spin_lock_irqsave(&in_lock, &flags);
    if (in_head == in_tail) {
        spin_unlock_irqrestore(&in_lock, flags);
        return 0;
    }
    if (ev) *ev = in_q[in_tail];
    in_tail = (in_tail + 1) % IN_QSIZE;
    spin_unlock_irqrestore(&in_lock, flags);
    return 1;
}

int input_pending(void) {
    return in_head != in_tail;
}

void input_key(uint8_t scancode, int pressed) {
    input_push(pressed ? IN_EV_KEY_PRESS : IN_EV_KEY_RELEASE, scancode, 0, 0);
}

void input_mouse(int16_t dx, int16_t dy, uint8_t buttons) {
    if (dx || dy)
        input_push(IN_EV_MOUSE_MOVE, buttons, dx, dy);
    else
        input_push(IN_EV_MOUSE_BTN, buttons, 0, 0);
}

int input_selftest(void) {
    struct input_event ev;
    /* Kuyruk boş başlamalı (boot'ta tuş basılmadı) */
    while (input_pop(&ev)) { /* varsa drenajla */ }
    /* Sentetik: 2 tuş + 1 fare olayı */
    if (!input_push(IN_EV_KEY_PRESS, 0x1E, 0, 0)) return -1;   /* A basıldı */
    if (!input_push(IN_EV_KEY_RELEASE, 0x1E, 0, 0)) return -1; /* A bırakıldı */
    if (!input_push(IN_EV_MOUSE_MOVE, 0, 5, -3)) return -1;
    int n = 0, ok = 1;
    uint8_t types[3];
    while (input_pop(&ev) && n < 3) { types[n++] = ev.type; }
    if (n != 3) ok = 0;
    if (ok && (types[0] != IN_EV_KEY_PRESS || types[1] != IN_EV_KEY_RELEASE ||
               types[2] != IN_EV_MOUSE_MOVE)) ok = 0;
    if (ok && (ev.x != 5 || ev.y != -3)) ok = 0; /* son olay fare */
    if (input_pending()) ok = 0;
    if (ok) {
        serial_puts("[18A] input hub inject/drain [PASS]\n");
        vga_puts("[18A] input hub inject/drain [PASS]\n");
        return 0;
    }
    serial_puts("[18A] input hub [FAIL]\n");
    vga_puts("[18A] input hub [FAIL]\n");
    return -1;
}
