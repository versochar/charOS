/* 45D: hub guc yonetimi — port durumu + asiri-akim + butce. */
#include "arch/x86_64/longmode.h"

#define HUBPOWER64_MAX_PORTS 16

static int hubpower64_state_tab[HUBPOWER64_MAX_PORTS];
static int hubpower64_oc[HUBPOWER64_MAX_PORTS];
static int hubpower64_ready = 0;

static void hubpower64_ensure(void) {
    int i;
    if (hubpower64_ready) return;
    for (i = 0; i < HUBPOWER64_MAX_PORTS; i++) {
        hubpower64_state_tab[i] = HUBPOWER64_ON;
        hubpower64_oc[i] = 0;
    }
    hubpower64_ready = 1;
}

int hubpower64_set(int port, int state) {
    hubpower64_ensure();
    if (port < 0 || port >= HUBPOWER64_MAX_PORTS) return -1;
    if (state < HUBPOWER64_OFF || state > HUBPOWER64_SUSPEND) return -1;
    if (hubpower64_oc[port]) return -2; /* OC kilitli: once temizle */
    hubpower64_state_tab[port] = state;
    return 0;
}

int hubpower64_state(int port) {
    hubpower64_ensure();
    if (port < 0 || port >= HUBPOWER64_MAX_PORTS) return -1;
    return hubpower64_state_tab[port];
}

int hubpower64_overcurrent(int port) {
    hubpower64_ensure();
    if (port < 0 || port >= HUBPOWER64_MAX_PORTS) return -1;
    hubpower64_oc[port] = 1;
    hubpower64_state_tab[port] = HUBPOWER64_OFF;
    return 0;
}

/* Hiz basina birim yuk (mA): LS/FS=100, HS=500, SS=900. */
int hubpower64_budget(int speed_mbps, int *ma_out) {
    int ma;
    if (speed_mbps <= 0) return -1;
    if (speed_mbps <= 12)
        ma = 100;
    else if (speed_mbps <= 480)
        ma = 500;
    else
        ma = 900;
    if (ma_out) *ma_out = ma;
    return 0;
}
