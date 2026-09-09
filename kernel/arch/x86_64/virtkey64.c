/* 55E: virtual keyboard — tus/fare enjeksiyonu + degistirici durumu. */
#include "arch/x86_64/longmode.h"

#define VIRTKEY64_MAXQ 32

struct virtkey64_ev {
    u32 keycode;
    int pressed;
};

static struct virtkey64_ev virtkey64_q[VIRTKEY64_MAXQ];
static int virtkey64_head = 0;
static int virtkey64_n = 0;
static u32 virtkey64_mod_state = 0;

static int virtkey64_push(u32 keycode, int pressed) {
    if (virtkey64_n >= VIRTKEY64_MAXQ) return -1;
    virtkey64_q[(virtkey64_head + virtkey64_n) % VIRTKEY64_MAXQ].keycode =
        keycode;
    virtkey64_q[(virtkey64_head + virtkey64_n) % VIRTKEY64_MAXQ].pressed =
        pressed;
    virtkey64_n++;
    return 0;
}

int virtkey64_press(u32 keycode) {
    if (keycode >= 0xE0 && keycode <= 0xE7)
        virtkey64_mod_state |= (1UL << (keycode - 0xE0));
    return virtkey64_push(keycode, 1);
}

int virtkey64_release(u32 keycode) {
    if (keycode >= 0xE0 && keycode <= 0xE7)
        virtkey64_mod_state &= ~(1UL << (keycode - 0xE0));
    return virtkey64_push(keycode, 0);
}

u32 virtkey64_mods(void) { return virtkey64_mod_state; }

int virtkey64_read(u32 *keycode, int *pressed) {
    if (!virtkey64_n) return -1;
    if (keycode) *keycode = virtkey64_q[virtkey64_head].keycode;
    if (pressed) *pressed = virtkey64_q[virtkey64_head].pressed;
    virtkey64_head = (virtkey64_head + 1) % VIRTKEY64_MAXQ;
    virtkey64_n--;
    return 0;
}
