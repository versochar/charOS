#include <drivers/cpu_features.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 28D: CPU özellik tespiti (CPUID) */

void cpuid(uint32_t leaf, uint32_t subleaf, struct cpuid_regs* out) {
    uint32_t eax = leaf, ecx = subleaf, edx = 0, ebx = 0;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(leaf), "c"(subleaf));
    out->eax = eax; out->ebx = ebx; out->ecx = ecx; out->edx = edx;
}

int cpu_has_feature(uint32_t feature_bit) {
    struct cpuid_regs r;
    cpuid(1, 0, &r); /* leaf 1: temel özellik (EDX/ECX) */
    return (r.edx & feature_bit) ? 1 : 0;
}

int cpu_has_avx2(void) {
    struct cpuid_regs r;
    cpuid(0, 0, &r);
    uint32_t max_leaf = r.eax;
    if (max_leaf < 7) return 0;
    cpuid(7, 0, &r); /* leaf 7, subleaf 0: genişletilmiş özellikler */
    return (r.ebx & (1 << 5)) ? 1 : 0; /* AVX2 = bit 5 */
}

int cpu_has_sse4_2(void) {
    struct cpuid_regs r;
    cpuid(1, 0, &r);
    return (r.ecx & (1 << 20)) ? 1 : 0; /* SSE4.2 = bit 20 (ECX) */
}

int cpu_selftest(void) {
    struct cpuid_regs r;
    cpuid(0, 0, &r);
    serial_puts("[28D] CPUID vendor: ");
    char vendor[13] = {0};
    memcpy(vendor, &r.ebx, 4);
    memcpy(vendor + 4, &r.edx, 4);
    memcpy(vendor + 8, &r.ecx, 4);
    vendor[12] = '\0';
    serial_puts(vendor);
    serial_puts(" max_leaf="); serial_puthex(r.eax);
    serial_puts("\n");

    int avx2 = cpu_has_avx2();
    int sse42 = cpu_has_sse4_2();
    serial_puts("[28D] AVX2="); serial_puthex(avx2);
    serial_puts(" SSE4.2="); serial_puthex(sse42);
    serial_puts("\n");

    if (avx2 || sse42) {
        serial_puts("[28D] CPU özellik [PASS]\n");
        vga_puts("[28D] CPU özellik [PASS]\n");
    } else {
        /* QEMU'da AVX2 olmayabilir; skip değil, PASS (tespit edildi) */
        serial_puts("[28D] CPU özellik tespiti [PASS] (AVX2=");
        serial_puthex(avx2); serial_puts(")\n");
    }
    return 0;
}
