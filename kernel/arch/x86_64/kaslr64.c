/* 32E: KASLR — entropi + kaydirma hesabi.
 * RDRAND varsa donanim, yoksa RDTSC yedegi. Saf C + 2 kucuk asm.
 */
#include "arch/x86_64/longmode.h"

u64 kaslr64_entropy(void) {
    u64 v = 0;
    unsigned char ok = 0;
    __asm__ volatile("rdrand %0; setc %1" : "=r"(v), "=r"(ok));
    if (ok && v) return v;
    __asm__ volatile("rdtsc; salq $32, %%rdx; orq %%rdx, %%rax; movq %%rax, %0"
                     : "=r"(v) :: "rax", "rdx");
    return v ? v : 0x9E3779B97F4A7C15ULL;
}

/* Kaydirma: [0, 1GB) araliginda, align hizali (2MB onerilir) */
u64 kaslr64_slide(u64 entropy, u64 align) {
    u64 mask;
    if (align < PMM64_FRAME) align = PMM64_FRAME;
    mask = (1ULL << 30) - align; /* 1GB pencere */
    return entropy & mask & ~(align - 1);
}

int kaslr64_verify(u64 load_base, u64 link_base, u64 slide) {
    if (load_base < link_base) return -1;
    return (load_base - link_base) == slide ? 0 : -1;
}
