/* 31D: long mode aktivasyon yardimcilari (CR0/CR4/EFER okuma) */
#include "arch/x86_64/longmode.h"

u64 longmode_get_cr0(void) {
    u64 v; __asm__ volatile("movq %%cr0, %0" : "=r"(v)); return v;
}
u64 longmode_get_cr4(void) {
    u64 v; __asm__ volatile("movq %%cr4, %0" : "=r"(v)); return v;
}
u64 longmode_get_efer(void) {
    u64 lo, hi;
    __asm__ volatile("mov $0xC0000080, %%ecx; rdmsr; movq %%rax, %0; movq %%rdx, %1"
                     : "=r"(lo), "=r"(hi) :: "rax", "rcx", "rdx");
    return (hi << 32) | (lo & 0xFFFFFFFFULL);
}

void longmode_enable_paging(u64 pml4_phys) {
    /* PML4 yukle; paging zaten aktif varsayilir (UEFI long mode'dayiz).
     * Gercek gecis boot64.s + UEFI loader'da olur; burada sadece CR3 guncellenir. */
    __asm__ volatile("movq %0, %%cr3" :: "r"(pml4_phys) : "memory");
}
