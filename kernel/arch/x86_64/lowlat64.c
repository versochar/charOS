/* 46H: low latency — periyot/uyanma hesabi + gercek-zaman ilkesi. */
#include "arch/x86_64/longmode.h"

static int lowlat64_rt = 0;

u32 lowlat64_period_frames(u32 rate, u32 latency_us) {
    u64 f;
    if (!rate || !latency_us) return 0;
    f = (u64)rate * latency_us / 1000000;
    if (!f) f = 1;
    /* 2'nin kuvvetine yuvarla (DMA dostu) */
    {
        u32 p = 1;
        while (p < f && p < 0x80000000UL) p <<= 1;
        return p;
    }
}

u32 lowlat64_latency_us(u32 rate, u32 period_frames) {
    if (!rate) return 0;
    return (u32)((u64)period_frames * 1000000 / rate);
}

u32 lowlat64_watermark(u32 period_frames) {
    return period_frames / 2; /* yari-periyot uyanma */
}

int lowlat64_policy(int realtime) {
    lowlat64_rt = realtime ? 1 : 0;
    return lowlat64_rt;
}
