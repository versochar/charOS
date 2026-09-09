/* 33F: scheduler affinity — task->CPU maskesi + en-az-yuklu secim. */
#include "arch/x86_64/longmode.h"

#define AFF64_MAX_TASKS 256

static u64 aff64_mask[AFF64_MAX_TASKS];
static u64 aff64_load[MAX_CPUS64];
static int aff64_ncpu = 1;
static int aff64_ready = 0;

int aff64_init(int ncpu) {
    int i;
    if (ncpu <= 0 || ncpu > MAX_CPUS64) return -1;
    for (i = 0; i < AFF64_MAX_TASKS; i++)
        aff64_mask[i] = (ncpu >= 64) ? ~0ULL : ((1ULL << ncpu) - 1);
    for (i = 0; i < MAX_CPUS64; i++) aff64_load[i] = 0;
    aff64_ncpu = ncpu;
    aff64_ready = 1;
    return 0;
}

int aff64_set(int task, u64 mask) {
    if (!aff64_ready || task < 0 || task >= AFF64_MAX_TASKS) return -1;
    mask &= (aff64_ncpu >= 64) ? ~0ULL : ((1ULL << aff64_ncpu) - 1);
    if (!mask) return -2; /* bos maske yasak */
    aff64_mask[task] = mask;
    return 0;
}

u64 aff64_get(int task) {
    if (task < 0 || task >= AFF64_MAX_TASKS) return 0;
    return aff64_mask[task];
}

int aff64_pick(int task) {
    u64 mask;
    int c, best = -1;
    u64 bestload = ~0ULL;
    if (!aff64_ready || task < 0 || task >= AFF64_MAX_TASKS) return -1;
    mask = aff64_mask[task];
    for (c = 0; c < aff64_ncpu; c++) {
        if (!(mask & (1ULL << c))) continue;
        if (aff64_load[c] < bestload) {
            bestload = aff64_load[c];
            best = c;
        }
    }
    if (best >= 0) aff64_load[best]++;
    return best;
}
