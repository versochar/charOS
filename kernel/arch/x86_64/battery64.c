/* 48G: battery — kapasite/gerilim/durum + kalan sure tahmini. */
#include "arch/x86_64/longmode.h"

static u64 battery64_design = 0;
static u64 battery64_full = 0;
static u64 battery64_cur = 0;
static u32 battery64_mv = 0;
static int battery64_charging = 0;
static int battery64_rate_mw = 0;

int battery64_update(u64 design, u64 full, u64 cur, u32 mv, int charging) {
    battery64_design = design;
    battery64_full = full;
    battery64_cur = cur > full ? full : cur;
    battery64_mv = mv;
    battery64_charging = charging ? 1 : 0;
    return 0;
}

/* Tuketim hizi disaridan (mW); pozitif = desarj. */
void battery64_set_rate(int mw) { battery64_rate_mw = mw; }

int battery64_pct(void) {
    if (!battery64_full) return 0;
    return (int)(battery64_cur * 100 / battery64_full);
}

int battery64_state(void) {
    if (!battery64_full) return 0;
    if (battery64_cur >= battery64_full) return 3;
    return battery64_charging ? 1 : 2;
}

u64 battery64_minutes(void) {
    u64 energy;
    if (battery64_rate_mw <= 0) return 0;
    if (battery64_charging) {
        if (battery64_cur >= battery64_full) return 0;
        energy = battery64_full - battery64_cur;
    } else {
        energy = battery64_cur;
    }
    return energy * 60 / (u64)battery64_rate_mw;
}
