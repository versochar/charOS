/* 55C: input v2 — cihazlar + olay kuyrugu + tekil yakalama. */
#include "arch/x86_64/longmode.h"

#define INPUTV264_MAX_DEV 8
#define INPUTV264_MAX_EV 128

struct inputv264_dev {
    int used;
    int caps;
    int grabbed;
};

struct inputv264_ev {
    int dev;
    int type;
    int code;
    int value;
    u64 time;
};

static struct inputv264_dev inputv264_devs[INPUTV264_MAX_DEV];
static struct inputv264_ev inputv264_q[INPUTV264_MAX_EV];
static int inputv264_head = 0;
static int inputv264_n = 0;
static int inputv264_grabbed = -1;

int inputv264_add(int caps) {
    int i;
    if (!caps) return -1;
    for (i = 0; i < INPUTV264_MAX_DEV; i++) {
        if (!inputv264_devs[i].used) {
            inputv264_devs[i].used = 1;
            inputv264_devs[i].caps = caps;
            inputv264_devs[i].grabbed = 0;
            return i;
        }
    }
    return -2;
}

int inputv264_emit(int dev, int type, int code, int value, u64 time) {
    if (dev < 0 || dev >= INPUTV264_MAX_DEV || !inputv264_devs[dev].used)
        return -1;
    if (inputv264_n >= INPUTV264_MAX_EV) return -2;
    inputv264_q[(inputv264_head + inputv264_n) % INPUTV264_MAX_EV].dev =
        dev;
    inputv264_q[(inputv264_head + inputv264_n) % INPUTV264_MAX_EV].type =
        type;
    inputv264_q[(inputv264_head + inputv264_n) % INPUTV264_MAX_EV].code =
        code;
    inputv264_q[(inputv264_head + inputv264_n) % INPUTV264_MAX_EV].value =
        value;
    inputv264_q[(inputv264_head + inputv264_n) % INPUTV264_MAX_EV].time =
        time;
    inputv264_n++;
    return 0;
}

int inputv264_poll(int *dev, int *type, int *code, int *value) {
    if (!inputv264_n) return -1;
    if (dev) *dev = inputv264_q[inputv264_head].dev;
    if (type) *type = inputv264_q[inputv264_head].type;
    if (code) *code = inputv264_q[inputv264_head].code;
    if (value) *value = inputv264_q[inputv264_head].value;
    inputv264_head = (inputv264_head + 1) % INPUTV264_MAX_EV;
    inputv264_n--;
    return 0;
}

int inputv264_grab(int dev) {
    int i;
    if (dev < 0 || dev >= INPUTV264_MAX_DEV || !inputv264_devs[dev].used)
        return -1;
    for (i = 0; i < INPUTV264_MAX_DEV; i++)
        inputv264_devs[i].grabbed = 0;
    inputv264_devs[dev].grabbed = 1;
    inputv264_grabbed = dev;
    return 0;
}
