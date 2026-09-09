/* 38D: capabilities v3 — gorev basina 3 kume + exec gecisi + sinir kumesi. */
#include "arch/x86_64/longmode.h"

#define CAPV364_MAX_TASKS 64

static u64 capv364_sets[CAPV364_MAX_TASKS][CAPV364_NSET];
static u64 capv364_bound = ~0ULL;
static int capv364_ready = 0;

static void capv364_ensure(void) {
    int t, s;
    if (capv364_ready) return;
    for (t = 0; t < CAPV364_MAX_TASKS; t++)
        for (s = 0; s < CAPV364_NSET; s++)
            capv364_sets[t][s] = (s == CAPV364_PERMITTED) ? ~0ULL : 0;
    capv364_ready = 1;
}

int capv364_set(int task, int which, u64 mask) {
    if (task < 0 || task >= CAPV364_MAX_TASKS) return -1;
    if (which < 0 || which >= CAPV364_NSET) return -1;
    capv364_ensure();
    if (which == CAPV364_PERMITTED) mask &= capv364_bound;
    capv364_sets[task][which] = mask;
    return 0;
}

u64 capv364_get(int task, int which) {
    if (task < 0 || task >= CAPV364_MAX_TASKS) return 0;
    if (which < 0 || which >= CAPV364_NSET) return 0;
    capv364_ensure();
    return capv364_sets[task][which];
}

int capv364_has(int task, int which, int cap) {
    if (cap < 0 || cap >= 64) return 0;
    return (capv364_get(task, which) & (1ULL << cap)) ? 1 : 0;
}

/* POSIX exec gecisi (sadelestirilmis):
 * P'(perm) = (P(inh) & F(perm)) | (F(inh) & bound)
 * P'(eff)  = P'(perm) (varsayilan; korunmus degil)
 * P'(inh)  = P(inh) */
int capv364_exec(int task, u64 file_perm, u64 file_inh) {
    u64 pinh, pnew;
    if (task < 0 || task >= CAPV364_MAX_TASKS) return -1;
    capv364_ensure();
    pinh = capv364_sets[task][CAPV364_INHERITABLE];
    pnew = (pinh & file_perm) | (file_inh & capv364_bound);
    capv364_sets[task][CAPV364_PERMITTED] = pnew;
    capv364_sets[task][CAPV364_EFFECTIVE] = pnew;
    return 0;
}

int capv364_bound_add(u64 mask) {
    capv364_ensure();
    capv364_bound &= mask; /* sinir yalnizca daralir */
    return 0;
}

u64 capv364_bound_get(void) {
    capv364_ensure();
    return capv364_bound;
}
