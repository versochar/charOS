/* 37A: ASLR userspace — yigin/mmap/exec rastgele tabanlari.
 * Tum araliklar 2GB alti (32-bit uyumlu sanal alan duzeni korunur).
 */
#include "arch/x86_64/longmode.h"

/* Yigin tepesi: 0xC0000000 yakin, 16B hizali, 8MB pencere */
u64 aslr64_stack_base(u64 entropy) {
    u64 off = (entropy % 512) * 0x4000ULL; /* 512 x 16K adim */
    return (0xC0000000ULL - off) & ~0xFULL;
}

/* mmap tabani: 0x40000000 yakin, 2MB hizali, 256MB pencere */
u64 aslr64_mmap_base(u64 entropy) {
    u64 off = (entropy % 128) * 0x200000ULL;
    return 0x40000000ULL + off;
}

/* PIE ise 64K hizali kaydirma, degilse sabit baglanti adresi */
u64 aslr64_exec_base(u64 entropy, int pie) {
    u64 off;
    if (!pie) return 0x1000000ULL; /* 16MB sabit */
    off = (entropy % 256) * 0x10000ULL;
    return 0x1000000ULL + off;
}
