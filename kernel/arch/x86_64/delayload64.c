/* 52G: delay load — tembel baglama saplamalari. */
#include "arch/x86_64/longmode.h"

#define DELAYLOAD64_MAX 64

struct delayload64_stub {
    int used;
    char sym[64];
    u64 addr; /* 0 = cozunmemis */
};

static struct delayload64_stub delayload64_tab[DELAYLOAD64_MAX];

static void delay_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int delay_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int delayload64_register(const char *sym) {
    int i;
    if (!sym) return -1;
    for (i = 0; i < DELAYLOAD64_MAX; i++) {
        if (delayload64_tab[i].used &&
            delay_str_eq(delayload64_tab[i].sym, sym))
            return 0;
    }
    for (i = 0; i < DELAYLOAD64_MAX; i++) {
        if (!delayload64_tab[i].used) {
            delayload64_tab[i].used = 1;
            delay_str_copy(delayload64_tab[i].sym, sym, 64);
            delayload64_tab[i].addr = 0;
            return 0;
        }
    }
    return -2;
}

int delayload64_resolve(const char *sym, u64 addr) {
    int i;
    if (!sym || !addr) return -1;
    for (i = 0; i < DELAYLOAD64_MAX; i++) {
        if (delayload64_tab[i].used &&
            delay_str_eq(delayload64_tab[i].sym, sym)) {
            delayload64_tab[i].addr = addr;
            return 0;
        }
    }
    return -2;
}

u64 delayload64_call(const char *sym) {
    int i;
    if (!sym) return 0;
    for (i = 0; i < DELAYLOAD64_MAX; i++) {
        if (delayload64_tab[i].used &&
            delay_str_eq(delayload64_tab[i].sym, sym))
            return delayload64_tab[i].addr; /* 0 = henuz baglanmamis */
    }
    return 0;
}
