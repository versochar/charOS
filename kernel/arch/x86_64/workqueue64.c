/* 33E: per-CPU workqueue — basit FIFO, kilitsiz tek-uretici varsayimi.
 * Cok-uretici gercek kullanimda caller spinlock alir (31x spinlock'a baglanir).
 */
#include "arch/x86_64/longmode.h"

struct wq64_item {
    int used;
    wq64_fn fn;
    void *arg;
};

static struct wq64_item wq64_pool[MAX_CPUS64][WQ64_MAX];
static int wq64_ready = 0;

int wq64_init(void) {
    int c, i;
    for (c = 0; c < MAX_CPUS64; c++)
        for (i = 0; i < WQ64_MAX; i++) {
            wq64_pool[c][i].used = 0;
            wq64_pool[c][i].fn = 0;
            wq64_pool[c][i].arg = 0;
        }
    wq64_ready = 1;
    return 0;
}

int wq64_enqueue(int cpu, wq64_fn fn, void *arg) {
    int i;
    if (!wq64_ready || cpu < 0 || cpu >= MAX_CPUS64 || !fn) return -1;
    for (i = 0; i < WQ64_MAX; i++) {
        if (!wq64_pool[cpu][i].used) {
            wq64_pool[cpu][i].used = 1;
            wq64_pool[cpu][i].fn = fn;
            wq64_pool[cpu][i].arg = arg;
            return 0;
        }
    }
    return -2; /* dolu */
}

int wq64_pending(int cpu) {
    int i, n = 0;
    if (cpu < 0 || cpu >= MAX_CPUS64) return 0;
    for (i = 0; i < WQ64_MAX; i++)
        if (wq64_pool[cpu][i].used) n++;
    return n;
}

int wq64_run(int cpu) {
    int i, ran = 0;
    if (!wq64_ready || cpu < 0 || cpu >= MAX_CPUS64) return -1;
    for (i = 0; i < WQ64_MAX; i++) {
        if (wq64_pool[cpu][i].used) {
            wq64_fn fn = wq64_pool[cpu][i].fn;
            void *arg = wq64_pool[cpu][i].arg;
            wq64_pool[cpu][i].used = 0;
            ran++;
            fn(arg);
        }
    }
    return ran;
}
