/* 48F: power button — basim suresi + ilke (kisa=aski, uzun=kapat). */
#include "arch/x86_64/longmode.h"

#define POWERBTN64_LONG_MS 4000
#define POWERBTN64_MAXQ 8

static int powerbtn64_q[POWERBTN64_MAXQ];
static int powerbtn64_head = 0;
static int powerbtn64_n = 0;

int powerbtn64_event(u32 duration_ms) {
    int ev = duration_ms >= POWERBTN64_LONG_MS ? 2 : 1;
    if (powerbtn64_n >= POWERBTN64_MAXQ) return -1;
    powerbtn64_q[(powerbtn64_head + powerbtn64_n) % POWERBTN64_MAXQ] = ev;
    powerbtn64_n++;
    return ev;
}

int powerbtn64_read(void) {
    int ev;
    if (!powerbtn64_n) return 0;
    ev = powerbtn64_q[powerbtn64_head];
    powerbtn64_head = (powerbtn64_head + 1) % POWERBTN64_MAXQ;
    powerbtn64_n--;
    return ev;
}

/* 0=hickimse, 1=askiya al, 2=kapat */
int powerbtn64_action(int ev) {
    if (ev == 1) return 1;
    if (ev == 2) return 2;
    return 0;
}
