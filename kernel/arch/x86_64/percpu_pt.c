/* 32F: per-CPU sayfa tablosu — her CPU'ya ayri PML4 kopyasi.
 * Klon pmm64'ten frame alir; switch CR3 yukler (caller CLI varsayar).
 */
#include "arch/x86_64/longmode.h"

static u64 *percpu_pml4[MAX_CPUS64];
static int percpu_ncpu = 0;

int percpu_pt_init(int ncpu) {
    int i;
    if (ncpu <= 0 || ncpu > MAX_CPUS64) return -1;
    for (i = 0; i < MAX_CPUS64; i++) percpu_pml4[i] = 0;
    percpu_ncpu = ncpu;
    return 0;
}

int percpu_pt_clone(int cpu, u64 *src_pml4) {
    u64 frame;
    u64 *dst;
    int i;
    if (cpu < 0 || cpu >= percpu_ncpu || !src_pml4) return -1;
    if (percpu_pml4[cpu]) return -2; /* zaten var */
    frame = pmm64_alloc_frame();
    if (!frame) return -3;
    dst = (u64 *)pmm64_frame_ptr(frame);
    for (i = 0; i < 512; i++) dst[i] = src_pml4[i];
    percpu_pml4[cpu] = dst;
    return 0;
}

u64 *percpu_pt_get(int cpu) {
    if (cpu < 0 || cpu >= percpu_ncpu) return 0;
    return percpu_pml4[cpu];
}

void percpu_pt_switch(int cpu) {
    u64 *pml4;
    u64 phys;
    int i;
    if (cpu < 0 || cpu >= percpu_ncpu) return;
    pml4 = percpu_pml4[cpu];
    if (!pml4) return;
    /* Skeleton identity varsayar: pointer == phys. Gercek kernel64'te
     * virt->phys cevrimi paging64 uzerinden olur. */
    (void)i;
    phys = (u64)pml4;
    __asm__ volatile("movq %0, %%cr3" :: "r"(phys) : "memory");
}
