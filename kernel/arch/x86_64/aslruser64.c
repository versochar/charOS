/* 52E: ASLR userspace — 37A cekirdegi ustu kullanici alanlari. */
#include "arch/x86_64/longmode.h"

u64 aslruser64_mmap_base(u64 entropy) {
    return aslr64_mmap_base(entropy);
}

u64 aslruser64_stack(u64 entropy) {
    return aslr64_stack_base(entropy ^ 0x535441434BULL);
}

u64 aslruser64_brk(u64 entropy) {
    return 0x20000000ULL + (entropy % (64ULL * 1024 * 1024));
}

u64 aslruser64_pie_bias(u64 entropy) {
    return kaslr64_slide(entropy, 2ULL * 1024 * 1024);
}
