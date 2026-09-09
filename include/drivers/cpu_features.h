#ifndef CHAROS_DRIVERS_CPU_FEATURES_H
#define CHAROS_DRIVERS_CPU_FEATURES_H

#include <stdint.h>

/* 28D: CPU özellik tespiti (CPUID üzerinden AVX2, SSE4.2, AES-NI vb).
 * Gerçek SIMD (AVX2) hızlandırma ileri aşama; temel: özellik varlık kaydı. */

/* CPUID sonuç yapıları */
struct cpuid_regs {
    uint32_t eax, ebx, ecx, edx;
};

/* CPUID çağrısı (leaf: EAX, subleaf: ECX) */
void cpuid(uint32_t leaf, uint32_t subleaf, struct cpuid_regs* out);

/* Temel özellik bayrakları (EDX bitleri) */
#define CPU_FEATURE_SSE2    (1 << 26)
#define CPU_FEATURE_SSE4_2  (1 << 20)
#define CPU_FEATURE_AVX     (1 << 28)
#define CPU_FEATURE_AVX2    (1 << 5)  /* EBX bit 5, leaf 7, subleaf 0 */

/* Özellik tespiti */
int cpu_has_feature(uint32_t feature_bit);

/* AVX2 (leaf 7, subleaf 0, EBX bit 5) tespiti */
int cpu_has_avx2(void);

/* SSE4.2 (leaf 1, ECX bit 20) tespiti */
int cpu_has_sse4_2(void);

/* Self-test: CPUID okuma + özellik doğrulama */
int cpu_selftest(void);

#endif
