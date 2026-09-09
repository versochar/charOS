/* 45H: sanal HID — enjeksiyon + rapor kuyrugu (8B boot raporu). */
#include "arch/x86_64/longmode.h"

#define VIRTHID64_MAXQ 16
#define VIRTHID64_REPORT 8

static unsigned char virthid64_desc[256];
static int virthid64_desclen = 0;
static int virthid64_ready = 0;
static unsigned char virthid64_q[VIRTHID64_MAXQ][VIRTHID64_REPORT];
static int virthid64_head = 0;
static int virthid64_n = 0;

int virthid64_create(const unsigned char *desc, int len) {
    int i;
    if (!desc || len <= 0 || len > 256) return -1;
    for (i = 0; i < len; i++) virthid64_desc[i] = desc[i];
    virthid64_desclen = len;
    virthid64_head = 0;
    virthid64_n = 0;
    virthid64_ready = 1;
    return 0;
}

static int virthid64_push(const unsigned char *rep) {
    int i;
    if (!virthid64_ready) return -1;
    if (virthid64_n >= VIRTHID64_MAXQ) return -2;
    for (i = 0; i < VIRTHID64_REPORT; i++)
        virthid64_q[(virthid64_head + virthid64_n) % VIRTHID64_MAXQ][i] =
            rep[i];
    virthid64_n++;
    return 0;
}

int virthid64_inject_key(u8 mod, const u8 keys[6]) {
    unsigned char rep[VIRTHID64_REPORT];
    int i;
    if (!keys) return -1;
    rep[0] = mod;
    rep[1] = 0;
    for (i = 0; i < 6; i++) rep[2 + i] = keys[i];
    return virthid64_push(rep);
}

int virthid64_inject_mouse(u8 buttons, int dx, int dy) {
    unsigned char rep[VIRTHID64_REPORT];
    int i;
    rep[0] = buttons;
    rep[1] = (unsigned char)(dx & 0xFF);
    rep[2] = (unsigned char)((dx >> 8) & 0xFF);
    rep[3] = (unsigned char)(dy & 0xFF);
    rep[4] = (unsigned char)((dy >> 8) & 0xFF);
    for (i = 5; i < VIRTHID64_REPORT; i++) rep[i] = 0;
    return virthid64_push(rep);
}

int virthid64_read(unsigned char *out, int max) {
    int i, n;
    if (!out || max <= 0) return -1;
    if (!virthid64_n) return 0;
    n = VIRTHID64_REPORT < max ? VIRTHID64_REPORT : max;
    for (i = 0; i < n; i++) out[i] = virthid64_q[virthid64_head][i];
    virthid64_head = (virthid64_head + 1) % VIRTHID64_MAXQ;
    virthid64_n--;
    return n;
}
