/* 31F: IDT64 + syscall MSR (STAR/LSTAR/SFMASK) skeleton */
#include "arch/x86_64/longmode.h"

struct idt64_entry { u64 low; u64 high; } __attribute__((packed));
static struct idt64_entry idt64[256];
static struct { u16 limit; u64 base; } __attribute__((packed)) idtr64;

static void idt64_set(int n, u64 handler) {
    idt64[n].low  = (handler & 0xFFFFULL) | (0x08ULL << 16) | (0x8E00ULL << 32) |
                    ((handler & 0xFFFF0000ULL) << 32);
    idt64[n].high = (handler >> 32);
}

static void default_handler64(void) {
    __asm__ volatile("iretq");
}

void idt64_init(void) {
    for (int i = 0; i < 256; i++) idt64_set(i, (u64)default_handler64);
    idtr64.limit = sizeof(idt64) - 1;
    idtr64.base = (u64)idt64;
    __asm__ volatile("lidt %0" :: "m"(idtr64) : "memory");
}

void syscall64_init(void) {
    /* MSR: STAR=0xC0000081, LSTAR=0xC0000082, SFMASK=0xC0000084, EFER=0xC0000080
     * Handler henuz yok; sadece SCE biti acilir (31 serisi skeleton). */
    __asm__ volatile(
        "mov $0xC0000080, %%ecx; rdmsr; bts $0, %%rax; wrmsr"
        ::: "rax", "rcx", "rdx", "memory");
}
