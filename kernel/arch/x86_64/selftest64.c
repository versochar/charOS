/* 53H: self-test — kayitli kontroller + sonuc sorgusu. */
#include "arch/x86_64/longmode.h"

#define SELFTEST64_MAX 64

struct selftest64_entry {
    int used;
    char name[48];
    int (*fn)(void);
    int result; /* -1=kosmadi, 0=gecti, >0 basarisiz kodu */
};

static struct selftest64_entry selftest64_tab[SELFTEST64_MAX];

static void selftest_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int selftest64_add(const char *name, int (*fn)(void)) {
    int i;
    if (!name || !fn) return -1;
    for (i = 0; i < SELFTEST64_MAX; i++) {
        if (!selftest64_tab[i].used) {
            selftest64_tab[i].used = 1;
            selftest_str_copy(selftest64_tab[i].name, name, 48);
            selftest64_tab[i].fn = fn;
            selftest64_tab[i].result = -1;
            return 0;
        }
    }
    return -2;
}

int selftest64_run(void) {
    int i, fails = 0;
    for (i = 0; i < SELFTEST64_MAX; i++) {
        if (!selftest64_tab[i].used) continue;
        selftest64_tab[i].result = selftest64_tab[i].fn();
        if (selftest64_tab[i].result != 0) fails++;
    }
    return fails;
}

int selftest64_result(int i, char *name_out, int max) {
    if (i < 0 || i >= SELFTEST64_MAX || !selftest64_tab[i].used)
        return -2;
    if (name_out && max > 0)
        selftest_str_copy(name_out, selftest64_tab[i].name, max);
    return selftest64_tab[i].result;
}
