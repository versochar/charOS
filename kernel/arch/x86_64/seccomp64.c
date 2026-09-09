/* 58B: seccomp filtre */
#include "arch/x86_64/longmode.h"

#define SECCOMP_MAX 128

struct seccomp_rule {
    int used;
    int syscall_nr;
    int action;
};

static struct seccomp_rule rules[SECCOMP_MAX];
static int seccomp_active = 0;

int seccomp64_load(void) {
    if (seccomp_active) return -1;
    seccomp_active = 1;
    int i;
    for (i = 0; i < SECCOMP_MAX; i++) rules[i].used = 0;
    return 0;
}

int seccomp64_add_rule(int nr, int action) {
    int i;
    if (!seccomp_active) return -1;
    for (i = 0; i < SECCOMP_MAX; i++) {
        if (!rules[i].used) {
            rules[i].used = 1;
            rules[i].syscall_nr = nr;
            rules[i].action = action;
            return 0;
        }
    }
    return -2;
}

int seccomp64_check(int nr) {
    int i;
    if (!seccomp_active) return 1;
    for (i = 0; i < SECCOMP_MAX; i++) {
        if (rules[i].used && rules[i].syscall_nr == nr) {
            return rules[i].action;
        }
    }
    return 1;
}

int seccomp64_unload(void) {
    if (!seccomp_active) return -1;
    seccomp_active = 0;
    return 0;
}
