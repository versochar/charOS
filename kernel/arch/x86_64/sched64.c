/* 47G: GPU scheduler — oncelikli baglamlar + kuantum + kesme. */
#include "arch/x86_64/longmode.h"

#define SCHED64_MAX_CTX 16

struct sched64_ctx {
    int used;
    int prio;
    u64 quantum_us;
    u64 pending;
    int preempted;
};

static struct sched64_ctx sched64_tab[SCHED64_MAX_CTX];

int sched64_ctx_add(int prio, u64 quantum_us) {
    int i;
    if (prio < 0 || prio > 7 || !quantum_us) return -1;
    for (i = 0; i < SCHED64_MAX_CTX; i++) {
        if (!sched64_tab[i].used) {
            sched64_tab[i].used = 1;
            sched64_tab[i].prio = prio;
            sched64_tab[i].quantum_us = quantum_us;
            sched64_tab[i].pending = 0;
            sched64_tab[i].preempted = 0;
            return i;
        }
    }
    return -2;
}

int sched64_submit(int ctx, u64 jobs) {
    if (ctx < 0 || ctx >= SCHED64_MAX_CTX || !sched64_tab[ctx].used)
        return -1;
    sched64_tab[ctx].pending += jobs;
    return 0;
}

/* Oncelik-sirali calistir (yuksek once once); donus tamamlanan is. */
u64 sched64_run(void) {
    int p, i;
    u64 done = 0;
    for (p = 7; p >= 0; p--) {
        for (i = 0; i < SCHED64_MAX_CTX; i++) {
            struct sched64_ctx *c = &sched64_tab[i];
            if (!c->used || c->prio != p || c->preempted) continue;
            done += c->pending;
            c->pending = 0;
        }
    }
    return done;
}

int sched64_preempt(int ctx) {
    if (ctx < 0 || ctx >= SCHED64_MAX_CTX || !sched64_tab[ctx].used)
        return -1;
    sched64_tab[ctx].preempted = 1;
    return 0;
}
