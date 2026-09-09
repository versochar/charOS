/* 49I: health test — kayitli kontroller + bit maskeli sonuc. */
#include "arch/x86_64/longmode.h"

#define HEALTH64_MAX 32

struct health64_check {
    int used;
    char name[32];
    int (*fn)(void);
};

static struct health64_check health64_tab[HEALTH64_MAX];
static u64 health64_last_mask = 0;

static void health_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int health64_add(const char *name, int (*fn)(void)) {
    int i;
    if (!name || !fn) return -1;
    for (i = 0; i < HEALTH64_MAX; i++) {
        if (!health64_tab[i].used) {
            health64_tab[i].used = 1;
            health_str_copy(health64_tab[i].name, name, 32);
            health64_tab[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

/* Basarisiz sayisi; maske: bit i = i. kontrol basarisiz. */
int health64_run(void) {
    int i, fails = 0;
    health64_last_mask = 0;
    for (i = 0; i < HEALTH64_MAX; i++) {
        if (!health64_tab[i].used) continue;
        if (health64_tab[i].fn() != 0) {
            fails++;
            if (i < 64) health64_last_mask |= (1ULL << i);
        }
    }
    return fails;
}

u64 health64_status(void) { return health64_last_mask; }
