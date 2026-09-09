/* 31C: 4-level paging64 - PML4/PDP/PD, 2MiB huge pages
 * 512MB VRAM + 2GB RAM hedefi: ilk 4GB identity map (2MiB sayfalar).
 * Freestanding, -m64. Dinamik alloc yok; tablo UEFI'den hazir gelir.
 */
#include "arch/x86_64/longmode.h"

void paging64_init(u64 *pml4) {
    for (int i = 0; i < PML4_ENTRIES; i++) pml4[i] = 0;
}

void paging64_map_2mb(u64 *pml4, u64 virt, u64 phys, u64 flags) {
    u64 pml4i = (virt >> 39) & 0x1FF;
    u64 pdpi  = (virt >> 30) & 0x1FF;
    u64 pdi   = (virt >> 21) & 0x1FF;
    /* Tablolar caller tarafindan hazirlanir; burada sadece entry yazilir.
     * PDP/PD pointerlari pml4 icinde sakli kabul edilir (identity). */
    u64 *pdp = (u64 *)(pml4[pml4i] & ~0xFFFULL);
    if (!pdp) return;
    u64 *pd = (u64 *)(pdp[pdpi] & ~0xFFFULL);
    if (!pd) return;
    pd[pdi] = (phys & ~0x1FFFFFULL) | (flags & ~PAGE_PS64) | PAGE_PS64 | PAGE_PRESENT64;
}

int paging64_is_longmode(void) {
    u64 efer;
    __asm__ volatile("mov $0xC0000080, %%ecx; rdmsr; salq $32, %%rdx; orq %%rdx, %%rax; movq %%rax, %0"
                     : "=r"(efer) :: "rax", "rcx", "rdx");
    return (efer & (1ULL << 8)) && (efer & (1ULL << 10)); /* LME+LMA */
}
