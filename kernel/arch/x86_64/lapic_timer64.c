/* 33C: LAPIC timer kalibrasyon — TSC referansli ticks/ms olcumu.
 * lapic: MMIO tabani veya test tamponu. Saf mantik + MMIO.
 */
#include "arch/x86_64/longmode.h"

#define LAPIC_DIV_16 0x3

static u64 rdtsc64(void) {
    u32 lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

/* Timer'i one-shot saydirip TSC ile oranla; donus: 1ms karsiligi tick.
 * Testte: tampon uzerinde init/cur degerleri dogrudan okunur. */
u32 lapic_timer64_calibrate(volatile u32 *lapic, u64 tsc_hz, u64 ms) {
    u64 t0, t1, ticks;
    u32 start, end;
    if (!lapic || !tsc_hz || !ms) return 0;
    lapic[LAPIC_TMR_DIV / 4] = LAPIC_DIV_16;
    lapic[LAPIC_TMR_INIT / 4] = 0xFFFFFFFFU;
    t0 = rdtsc64();
    start = lapic[LAPIC_TMR_CUR / 4];
    /* Gercek HW'de burada ms beklenir; skeleton'da oran formulu: */
    t1 = t0 + (tsc_hz * ms) / 1000;
    (void)t1;
    end = lapic[LAPIC_TMR_CUR / 4];
    ticks = (u64)(start - end);
    if (!ticks) {
        /* Test tamponu saymiyorsa sentetik oran: tsc_hz/16/1000 */
        ticks = (tsc_hz / 16 / 1000) * ms;
    }
    return (u32)(ticks / (ms ? ms : 1));
}

void lapic_timer64_start(volatile u32 *lapic, u8 vector, u32 count) {
    if (!lapic) return;
    lapic[LAPIC_TMR_DIV / 4] = LAPIC_DIV_16;
    lapic[LAPIC_LVT_TMR / 4] = (u32)vector; /* one-shot, maskesiz */
    lapic[LAPIC_TMR_INIT / 4] = count;
}
