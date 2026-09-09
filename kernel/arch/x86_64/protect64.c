/* 31I: stack canary + relocator check */
#include "arch/x86_64/longmode.h"

u64 __stack_chk_guard64 = 0x595E9F87394ECB9FULL;

void stack_protector64_init(void) {
    /* RDRAND varsa gercek random, yoksa sabit guard (QEMU'da deterministik) */
    u64 v = 0;
    unsigned char ok = 0;
    __asm__ volatile("rdrand %0; setc %1" : "=r"(v), "=r"(ok));
    if (ok) __stack_chk_guard64 = v;
}

int relocator64_check(u64 load_base, u64 link_base) {
    /* PIE check: yuklenen adres link adresinden 2MB hizali farkli olabilir */
    if (load_base == link_base) return 0;
    u64 diff = load_base > link_base ? load_base - link_base : link_base - load_base;
    if (diff & 0x1FFFFFULL) return -1; /* 2MB hizasiz */
    return 1; /* relocated */
}
