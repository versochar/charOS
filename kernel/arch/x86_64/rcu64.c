/* 33H: RCU skeleton — epoch tabanli grace period, tek-yazar varsayimi. */
#include "arch/x86_64/longmode.h"

#define RCU64_MAX_CB 64

struct rcu64_cb {
    int used;
    void (*fn)(void *);
    void *arg;
    u64 epoch;
};

static struct rcu64_cb rcu64_cbs[RCU64_MAX_CB];
static u64 rcu64_epoch = 0;
static u64 rcu64_active_readers = 0;
static int rcu64_ready = 0;

void rcu64_init(void) {
    int i;
    for (i = 0; i < RCU64_MAX_CB; i++) {
        rcu64_cbs[i].used = 0;
        rcu64_cbs[i].fn = 0;
        rcu64_cbs[i].arg = 0;
        rcu64_cbs[i].epoch = 0;
    }
    rcu64_epoch = 0;
    rcu64_active_readers = 0;
    rcu64_ready = 1;
}

void rcu64_read_lock(void) {
    if (!rcu64_ready) return;
    rcu64_active_readers++;
}

void rcu64_read_unlock(void) {
    if (!rcu64_ready || !rcu64_active_readers) return;
    rcu64_active_readers--;
}

void rcu64_call(void (*fn)(void *), void *arg) {
    int i;
    if (!rcu64_ready || !fn) return;
    for (i = 0; i < RCU64_MAX_CB; i++) {
        if (!rcu64_cbs[i].used) {
            rcu64_cbs[i].used = 1;
            rcu64_cbs[i].fn = fn;
            rcu64_cbs[i].arg = arg;
            rcu64_cbs[i].epoch = rcu64_epoch;
            return;
        }
    }
}

void rcu64_quiescent(int cpu) {
    (void)cpu;
    if (!rcu64_ready) return;
    /* Tum CPU'lar susunca epoch ilerler; skeleton'da tek cagri = 1 adim */
    rcu64_epoch++;
}

int rcu64_process(void) {
    int i, ran = 0;
    if (!rcu64_ready) return -1;
    if (rcu64_active_readers) return 0; /* okuyucular varken bekle */
    for (i = 0; i < RCU64_MAX_CB; i++) {
        if (rcu64_cbs[i].used && rcu64_cbs[i].epoch < rcu64_epoch) {
            void (*fn)(void *) = rcu64_cbs[i].fn;
            void *arg = rcu64_cbs[i].arg;
            rcu64_cbs[i].used = 0;
            ran++;
            fn(arg);
        }
    }
    return ran;
}
