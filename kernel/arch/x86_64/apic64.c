/* 31H: APIC64 init skeleton (x2APIC MSR tabanli, mevcut 32-bit apic.c'yi bozmaz) */
#include "arch/x86_64/longmode.h"

#define IA32_APIC_BASE 0x1B
#define LAPIC_SVR 0xF0

static u64 rdmsr64(u32 msr) {
    u32 lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | lo;
}

u64 apic64_base(void) {
    return rdmsr64(IA32_APIC_BASE) & ~0xFFFULL;
}

int apic64_init(void) {
    u64 base = apic64_base();
    if (!base) return -1;
    /* SVR enable: 32-bit apic.c ile ayni politika, sadece 64-bit pointer */
    volatile u32 *svr = (volatile u32 *)(base + LAPIC_SVR);
    *svr = 0x1FF;
    return 0;
}
