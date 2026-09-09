/* 48D: cpufreq — P-durumlari + yuk-bazli yonetici. */
#include "arch/x86_64/longmode.h"

#define CPUFREQ64_MAX 16

struct cpufreq64_state {
    u32 mhz;
    u32 mw;
};

static struct cpufreq64_state cpufreq64_tab[CPUFREQ64_MAX];
static int cpufreq64_n = 0;
static u32 cpufreq64_cur = 0;

int cpufreq64_add_state(u32 mhz, u32 mw) {
    int i, j;
    if (!mhz) return -1;
    if (cpufreq64_n >= CPUFREQ64_MAX) return -2;
    /* MHz sirali ekle */
    for (i = 0; i < cpufreq64_n; i++) {
        if (cpufreq64_tab[i].mhz == mhz) {
            cpufreq64_tab[i].mw = mw;
            return 0;
        }
    }
    for (i = 0; i < cpufreq64_n; i++) {
        if (cpufreq64_tab[i].mhz > mhz) break;
    }
    for (j = cpufreq64_n; j > i; j--) cpufreq64_tab[j] = cpufreq64_tab[j - 1];
    cpufreq64_tab[i].mhz = mhz;
    cpufreq64_tab[i].mw = mw;
    cpufreq64_n++;
    if (!cpufreq64_cur) cpufreq64_cur = mhz;
    return 0;
}

/* ondemand: yuk>=80 en yuksek, <=20 en dusuk, arasi oranli. */
int cpufreq64_governor(int load_pct) {
    int idx;
    if (!cpufreq64_n) return -1;
    if (load_pct < 0) load_pct = 0;
    if (load_pct > 100) load_pct = 100;
    if (load_pct >= 80) return (int)cpufreq64_tab[cpufreq64_n - 1].mhz;
    if (load_pct <= 20) return (int)cpufreq64_tab[0].mhz;
    idx = (load_pct - 20) * cpufreq64_n / 60;
    if (idx >= cpufreq64_n) idx = cpufreq64_n - 1;
    return (int)cpufreq64_tab[idx].mhz;
}

int cpufreq64_set(u32 mhz) {
    int i;
    for (i = 0; i < cpufreq64_n; i++) {
        if (cpufreq64_tab[i].mhz == mhz) {
            cpufreq64_cur = mhz;
            return 0;
        }
    }
    return -1;
}

u32 cpufreq64_get(void) { return cpufreq64_cur; }
