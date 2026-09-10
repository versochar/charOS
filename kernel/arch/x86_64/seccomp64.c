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
static int seccomp_strict = 0;

static int seccomp_find(int nr) {
    int i;
    for (i = 0; i < SECCOMP_MAX; i++)
        if (rules[i].used && rules[i].syscall_nr == nr)
            return i;
    return -1;
}

int seccomp64_load(void) {
    int i;
    if (seccomp_active) return -1;
    seccomp_active = 1;
    for (i = 0; i < SECCOMP_MAX; i++) rules[i].used = 0;
    return 0;
}

int seccomp64_unload(void) {
    if (!seccomp_active) return -1;
    seccomp_active = 0;
    return 0;
}

int seccomp64_add_rule(int nr, int action) {
    int i;
    if (!seccomp_active) return -1;
    if (seccomp_find(nr) >= 0) return -2; /* ayni syscall iki kez */
    for (i = 0; i < SECCOMP_MAX; i++) {
        if (!rules[i].used) {
            rules[i].used = 1;
            rules[i].syscall_nr = nr;
            rules[i].action = action;
            return 0;
        }
    }
    return -3;
}

/* --- 37D: seccomp kural kumesi --- */
int seccomp64_add(int nr, int allow) {
    int i;
    if (seccomp_find(nr) >= 0) return -2;
    for (i = 0; i < SECCOMP_MAX; i++) {
        if (!rules[i].used) {
            rules[i].used = 1;
            rules[i].syscall_nr = nr;
            rules[i].action = allow;
            return 0;
        }
    }
    return -3;
}

void seccomp64_strict(int on) {
    seccomp_strict = on ? 1 : 0;
}

int seccomp64_count(void) {
    int i, c = 0;
    for (i = 0; i < SECCOMP_MAX; i++)
        if (rules[i].used) c++;
    return c;
}

int seccomp64_check(int nr) {
    int i = seccomp_find(nr);
    if (i >= 0) return rules[i].action;
    /* kural yok: kati moddaysa engelle, degilse serbest */
    return seccomp_strict ? 0 : 1;
}