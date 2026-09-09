/* 48E: cpuidle — C-durumlari + gecikme-bazli secim + kalis suresi. */
#include "arch/x86_64/longmode.h"

#define CPUIDLE64_MAX 8

struct cpuidle64_state {
    int used;
    char name[16];
    u32 latency_us;
    u32 power;
    u64 residency_us;
};

static struct cpuidle64_state cpuidle64_tab[CPUIDLE64_MAX];

static void idle_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int cpuidle64_add_state(const char *name, u32 latency_us, u32 power) {
    int i;
    if (!name) return -1;
    for (i = 0; i < CPUIDLE64_MAX; i++) {
        if (!cpuidle64_tab[i].used) {
            cpuidle64_tab[i].used = 1;
            idle_str_copy(cpuidle64_tab[i].name, name, 16);
            cpuidle64_tab[i].latency_us = latency_us;
            cpuidle64_tab[i].power = power;
            cpuidle64_tab[i].residency_us = 0;
            return i;
        }
    }
    return -2;
}

/* Bosta kalma suresinin yarisindan az gecikmeli en derin durum. */
int cpuidle64_pick(u32 idle_us) {
    int i, best = -1;
    for (i = 0; i < CPUIDLE64_MAX; i++) {
        if (!cpuidle64_tab[i].used) continue;
        if (cpuidle64_tab[i].latency_us * 2 > idle_us) continue;
        if (best < 0 ||
            cpuidle64_tab[i].latency_us > cpuidle64_tab[best].latency_us)
            best = i;
    }
    return best;
}

int cpuidle64_residency(int idx, u64 us) {
    if (idx < 0 || idx >= CPUIDLE64_MAX || !cpuidle64_tab[idx].used)
        return -1;
    cpuidle64_tab[idx].residency_us += us;
    return 0;
}
