/* 44F: guc tasarrufu — kip + DTIM uyanma karari + tasarruf sayaci. */
#include "arch/x86_64/longmode.h"

static int powersave64_cur = POWERSAVE64_ACTIVE;
static u64 powersave64_saved = 0;

int powersave64_set(int mode) {
    if (mode < POWERSAVE64_ACTIVE || mode > POWERSAVE64_DEEP) return -1;
    powersave64_cur = mode;
    return 0;
}

int powersave64_mode(void) { return powersave64_cur; }

/* DTIM isareti: sayac 0 ve tamponlu veri varsa uyan (1), yoksa uyu (0). */
int powersave64_beacon(int dtim_count, int buffered) {
    if (powersave64_cur == POWERSAVE64_ACTIVE) return 1;
    if (dtim_count == 0 && buffered) return 1;
    /* Uyku: tasarrufa isle (skeleton: 100ms isaret basina) */
    powersave64_saved += (powersave64_cur == POWERSAVE64_DEEP) ? 100000
                                                              : 20000;
    return 0;
}

u64 powersave64_saved_us(void) { return powersave64_saved; }
