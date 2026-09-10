#include "arch/x86_64/longmode.h"
#include "arch/x86_64/drvhal.h"

#define DRV_MAX 64

typedef struct {
    u64 hdl;
    int type;
    int active;
    u64 last_cmd;
} drv_slot_t;

static int drv_inited = 0;
static u64 drv_ctr = 0;
static drv_slot_t drvs[DRV_MAX];

static drv_slot_t *find_drv(u64 hdl) {
    for (int i = 0; i < DRV_MAX; i++) {
        if (drvs[i].active && drvs[i].hdl == hdl) return &drvs[i];
    }
    return 0;
}

int drvhal64_init(void) {
    drv_inited = 1;
    drv_ctr = 0;
    for (int i = 0; i < DRV_MAX; i++) {
        drvs[i].hdl = 0; drvs[i].type = 0; drvs[i].active = 0; drvs[i].last_cmd = 0;
    }
    return 0;
}

int drvhal64_register(int type, u64 *out_hdl) {
    if (!drv_inited || !out_hdl || type < 0 || type > 2) return -1;
    for (int i = 0; i < DRV_MAX; i++) {
        if (!drvs[i].active) {
            drv_ctr++;
            drvs[i].hdl = drv_ctr;
            drvs[i].type = type;
            drvs[i].active = 1;
            drvs[i].last_cmd = 0;
            *out_hdl = drv_ctr;
            return 0;
        }
    }
    return -1;
}

int drvhal64_unregister(u64 hdl) {
    drv_slot_t *d = find_drv(hdl);
    if (!d) return -1;
    d->active = 0; d->hdl = 0; d->last_cmd = 0;
    return 0;
}

int drvhal64_ioctl(u64 hdl, u64 cmd, u64 arg) {
    (void)arg;
    drv_slot_t *d = find_drv(hdl);
    if (!d) return -1;
    d->last_cmd = cmd;
    return 0;
}

int drvhal64_state(u64 hdl, int *out_state) {
    drv_slot_t *d = find_drv(hdl);
    if (!d || !out_state) return -1;
    *out_state = 1;
    return 0;
}
