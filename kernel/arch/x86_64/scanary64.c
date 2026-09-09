/* 37C: kernel stack canary — sifir-olmayan 64-bit deger + per-CPU tablo. */
#include "arch/x86_64/longmode.h"

static u64 scanary64_cpu[MAX_CPUS64];

u64 scanary64_gen(u64 entropy) {
    u64 v = entropy ? entropy : 0x9E3779B97F4A7C15ULL;
    v |= v >> 32; /* ust yari sifirsa doldur */
    if (!v) v = 0xA5A5A5A5A5A5A5A5ULL;
    return v;
}

int scanary64_verify(u64 stored, u64 current) {
    return stored == current ? 0 : -1;
}

int scanary64_set_cpu(int cpu, u64 val) {
    if (cpu < 0 || cpu >= MAX_CPUS64 || !val) return -1;
    scanary64_cpu[cpu] = val;
    return 0;
}

u64 scanary64_get_cpu(int cpu) {
    if (cpu < 0 || cpu >= MAX_CPUS64) return 0;
    return scanary64_cpu[cpu];
}
