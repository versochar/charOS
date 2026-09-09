/* 37E: capabilities audit — yetki maskesi + denetim halkasi. */
#include "arch/x86_64/longmode.h"

#define CAPAUDIT64_MAX_LOG 64

struct capaudit64_rec {
    int used;
    int pid;
    int cap;
    int granted;
};

static u64 capaudit64_mask = 0;
static struct capaudit64_rec capaudit64_recs[CAPAUDIT64_MAX_LOG];
static int capaudit64_head = 0;
static int capaudit64_n = 0;

int capaudit64_grant(int cap) {
    if (cap < 0 || cap >= 64) return -1;
    capaudit64_mask |= (1ULL << cap);
    return 0;
}

int capaudit64_revoke(int cap) {
    if (cap < 0 || cap >= 64) return -1;
    capaudit64_mask &= ~(1ULL << cap);
    return 0;
}

int capaudit64_has(int cap) {
    if (cap < 0 || cap >= 64) return 0;
    return (capaudit64_mask & (1ULL << cap)) ? 1 : 0;
}

int capaudit64_log(int pid, int cap, int granted) {
    struct capaudit64_rec *r = &capaudit64_recs[capaudit64_head];
    r->used = 1;
    r->pid = pid;
    r->cap = cap;
    r->granted = granted ? 1 : 0;
    capaudit64_head = (capaudit64_head + 1) % CAPAUDIT64_MAX_LOG;
    if (capaudit64_n < CAPAUDIT64_MAX_LOG) capaudit64_n++;
    return 0;
}

int capaudit64_read(int i, int *pid, int *cap, int *granted) {
    int idx;
    if (i < 0 || i >= capaudit64_n) return -1;
    /* En yeniden eskiye: i=0 son kayit */
    idx = (capaudit64_head - 1 - i + CAPAUDIT64_MAX_LOG * 2) %
          CAPAUDIT64_MAX_LOG;
    if (!capaudit64_recs[idx].used) return -2;
    if (pid) *pid = capaudit64_recs[idx].pid;
    if (cap) *cap = capaudit64_recs[idx].cap;
    if (granted) *granted = capaudit64_recs[idx].granted;
    return 0;
}
