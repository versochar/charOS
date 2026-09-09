/* 42I: rescue mode — tek kullanicili kurtarma: ac/kabuk/cik. */
#include "arch/x86_64/longmode.h"

static int rescue64_on = 0;
static char rescue64_reason[96];

int rescue64_enter(const char *reason) {
    int i;
    if (rescue64_on) return -1; /* zaten icerde */
    for (i = 0; reason && reason[i] && i < 95; i++)
        rescue64_reason[i] = reason[i];
    rescue64_reason[i] = 0;
    rescue64_on = 1;
    return 0;
}

int rescue64_shell_ok(void) {
    return rescue64_on ? 1 : 0;
}

int rescue64_exit(void) {
    if (!rescue64_on) return -1;
    rescue64_on = 0;
    rescue64_reason[0] = 0;
    return 0;
}

int rescue64_active(void) { return rescue64_on; }
