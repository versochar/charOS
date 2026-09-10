#include "arch/x86_64/longmode.h"
#include "arch/x86_64/virt.h"

#define VIRT_MAX 8
#define VIRT_Q 4

typedef struct {
    u64 dev;
    int type;
    u64 pending[VIRT_Q];
    int active;
} virt_dev_t;

static int virt_inited = 0;
static u64 virt_ctr = 0;
static virt_dev_t vdevs[VIRT_MAX];

static virt_dev_t *find_dev(u64 dev) {
    for (int i = 0; i < VIRT_MAX; i++) {
        if (vdevs[i].active && vdevs[i].dev == dev) return &vdevs[i];
    }
    return 0;
}

int virt64_init(void) {
    virt_inited = 1;
    virt_ctr = 0;
    for (int i = 0; i < VIRT_MAX; i++) {
        vdevs[i].dev = 0; vdevs[i].active = 0;
        for (int q = 0; q < VIRT_Q; q++) vdevs[i].pending[q] = 0;
    }
    return 0;
}

int virt64_add(int type, u64 *out_dev) {
    if (!virt_inited || !out_dev || type < 0 || type > 2) return -1;
    for (int i = 0; i < VIRT_MAX; i++) {
        if (!vdevs[i].active) {
            virt_ctr++;
            vdevs[i].dev = virt_ctr;
            vdevs[i].type = type;
            vdevs[i].active = 1;
            for (int q = 0; q < VIRT_Q; q++) vdevs[i].pending[q] = 0;
            *out_dev = virt_ctr;
            return 0;
        }
    }
    return -1;
}

int virt64_del(u64 dev) {
    virt_dev_t *d = find_dev(dev);
    if (!d) return -1;
    d->active = 0; d->dev = 0;
    return 0;
}

int virt64_kick(u64 dev, u64 q) {
    virt_dev_t *d = find_dev(dev);
    if (!d || q >= VIRT_Q) return -1;
    d->pending[q]++;
    return 0;
}

int virt64_poll(u64 dev, u64 q, u64 *out_pending) {
    virt_dev_t *d = find_dev(dev);
    if (!d || q >= VIRT_Q || !out_pending) return -1;
    *out_pending = d->pending[q];
    return 0;
}

int virt64_ack(u64 dev, u64 q) {
    virt_dev_t *d = find_dev(dev);
    if (!d || q >= VIRT_Q) return -1;
    if (d->pending[q] == 0) return -1;
    d->pending[q]--;
    return 0;
}
