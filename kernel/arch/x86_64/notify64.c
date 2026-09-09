/* 35E: basincl bildirimi — esik callback kaydi + yoklama atesi. */
#include "arch/x86_64/longmode.h"

struct notify64_entry {
    int used;
    void (*fn)(int level);
    int threshold;
    int fired;
};

static struct notify64_entry notify64_tab[NOTIFY64_MAX];

int notify64_register(void (*fn)(int level), int threshold) {
    int i;
    if (!fn || threshold < 0 || threshold > 100) return -1;
    for (i = 0; i < NOTIFY64_MAX; i++) {
        if (!notify64_tab[i].used) {
            notify64_tab[i].used = 1;
            notify64_tab[i].fn = fn;
            notify64_tab[i].threshold = threshold;
            notify64_tab[i].fired = 0;
            return 0;
        }
    }
    return -2;
}

int notify64_poll(void) {
    int level = pressure64_level();
    int n = 0, i;
    for (i = 0; i < NOTIFY64_MAX; i++) {
        if (!notify64_tab[i].used) continue;
        if (level >= notify64_tab[i].threshold &&
            !notify64_tab[i].fired) {
            notify64_tab[i].fired = 1;
            notify64_tab[i].fn(level);
            n++;
        } else if (level < notify64_tab[i].threshold) {
            notify64_tab[i].fired = 0; /* dusunce yeniden kurulur */
        }
    }
    return n;
}
