/* 49D: battery status — anlik gorunum + bitis tahmini. */
#include "arch/x86_64/longmode.h"

static int battstat64_pct_v = 0;
static int battstat64_charging_v = 0;
static u64 battstat64_rate = 0;
static u64 battstat64_cap = 0;

int battstat64_snapshot(int pct, int charging, u64 rate_mw, u64 cap_mwh) {
    if (pct < 0 || pct > 100) return -1;
    battstat64_pct_v = pct;
    battstat64_charging_v = charging ? 1 : 0;
    battstat64_rate = rate_mw;
    battstat64_cap = cap_mwh;
    return 0;
}

int battstat64_pct(void) { return battstat64_pct_v; }

int battstat64_minutes(void) {
    u64 energy;
    if (!battstat64_rate) return 0;
    if (battstat64_charging_v)
        energy = battstat64_cap * (u64)(100 - battstat64_pct_v) / 100;
    else
        energy = battstat64_cap * (u64)battstat64_pct_v / 100;
    return energy * 60 / battstat64_rate;
}
