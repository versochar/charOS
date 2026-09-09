/* 53G: module hotplug — tak/cikar + cihaz olay kuyrugu. */
#include "arch/x86_64/longmode.h"

#define HOTPLUG64_MAX_MOD 32
#define HOTPLUG64_MAX_EV 32

struct hotplug64_mod {
    int used;
    char name[48];
    int refcount;
};

struct hotplug64_ev {
    char dev[48];
    int action; /* 0=eklendi, 1=kaldirildi */
};

static struct hotplug64_mod hotplug64_mods[HOTPLUG64_MAX_MOD];
static struct hotplug64_ev hotplug64_q[HOTPLUG64_MAX_EV];
static int hotplug64_head = 0;
static int hotplug64_n = 0;

static void hotplug_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct hotplug64_mod *hotplug64_find(const char *name) {
    int i, j;
    for (i = 0; i < HOTPLUG64_MAX_MOD; i++) {
        int same;
        if (!hotplug64_mods[i].used) continue;
        same = 1;
        for (j = 0; name[j] || hotplug64_mods[i].name[j]; j++) {
            if (name[j] != hotplug64_mods[i].name[j]) {
                same = 0;
                break;
            }
        }
        if (same) return &hotplug64_mods[i];
    }
    return 0;
}

int hotplug64_insert(const char *name) {
    struct hotplug64_mod *m = hotplug64_find(name);
    int i;
    if (!name) return -1;
    if (m) {
        m->refcount++;
        return 0;
    }
    for (i = 0; i < HOTPLUG64_MAX_MOD; i++) {
        if (!hotplug64_mods[i].used) {
            hotplug64_mods[i].used = 1;
            hotplug_str_copy(hotplug64_mods[i].name, name, 48);
            hotplug64_mods[i].refcount = 1;
            return 0;
        }
    }
    return -2;
}

int hotplug64_remove(const char *name) {
    struct hotplug64_mod *m = hotplug64_find(name);
    if (!m) return -1;
    if (m->refcount > 1) {
        m->refcount--;
        return -2; /* kullanimda */
    }
    m->used = 0;
    m->refcount = 0;
    return 0;
}

int hotplug64_event(const char *dev, int action) {
    if (!dev || (action != 0 && action != 1)) return -1;
    if (hotplug64_n >= HOTPLUG64_MAX_EV) return -2;
    hotplug_str_copy(
        hotplug64_q[(hotplug64_head + hotplug64_n) % HOTPLUG64_MAX_EV].dev,
        dev, 48);
    hotplug64_q[(hotplug64_head + hotplug64_n) % HOTPLUG64_MAX_EV].action =
        action;
    hotplug64_n++;
    return 0;
}

int hotplug64_poll(char *dev_out, int max, int *action_out) {
    int i;
    if (!hotplug64_n) return -1;
    if (dev_out && max > 0) {
        for (i = 0; hotplug64_q[hotplug64_head].dev[i] && i + 1 < max;
             i++)
            dev_out[i] = hotplug64_q[hotplug64_head].dev[i];
        dev_out[i] = 0;
    }
    if (action_out) *action_out = hotplug64_q[hotplug64_head].action;
    hotplug64_head = (hotplug64_head + 1) % HOTPLUG64_MAX_EV;
    hotplug64_n--;
    return 0;
}
