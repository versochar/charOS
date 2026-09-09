/* 54I: node skeleton — modul kaydi + zamanlayici/ertelenmis kuyruk. */
#include "arch/x86_64/longmode.h"

#define NODE64_MAX_MOD 16
#define NODE64_MAX_TIMERS 32
#define NODE64_MAX_TICKS 32

static char node64_mods[NODE64_MAX_MOD][48];
static int node64_nmods = 0;

struct node64_timer {
    int used;
    u64 at;
    int cb;
};

static struct node64_timer node64_timers[NODE64_MAX_TIMERS];
static int node64_ticks[NODE64_MAX_TICKS];
static int node64_nticks = 0;

static void node_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int node_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int node64_require(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < node64_nmods; i++) {
        if (node_str_eq(node64_mods[i], name)) return i;
    }
    if (node64_nmods >= NODE64_MAX_MOD) return -2;
    node_str_copy(node64_mods[node64_nmods], name, 48);
    return node64_nmods++;
}

int node64_set_timeout(u64 ms, int cb, u64 now) {
    int i;
    if (cb < 0) return -1;
    for (i = 0; i < NODE64_MAX_TIMERS; i++) {
        if (!node64_timers[i].used) {
            node64_timers[i].used = 1;
            node64_timers[i].at = now + ms;
            node64_timers[i].cb = cb;
            return 0;
        }
    }
    return -2;
}

int node64_next_tick(int cb) {
    if (cb < 0) return -1;
    if (node64_nticks >= NODE64_MAX_TICKS) return -2;
    node64_ticks[node64_nticks++] = cb;
    return 0;
}

/* Once nextTick kuyrugu, sonra vadesi gelen zamanlayicilar. */
int node64_tick(u64 now, int *fired, int max) {
    int n = 0, i;
    if (!fired || max <= 0) return -1;
    while (node64_nticks > 0 && n < max) {
        fired[n++] = node64_ticks[0];
        for (i = 0; i + 1 < node64_nticks; i++)
            node64_ticks[i] = node64_ticks[i + 1];
        node64_nticks--;
    }
    for (i = 0; i < NODE64_MAX_TIMERS && n < max; i++) {
        if (!node64_timers[i].used) continue;
        if (node64_timers[i].at > now) continue;
        fired[n++] = node64_timers[i].cb;
        node64_timers[i].used = 0;
    }
    return n;
}
