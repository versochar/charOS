/* 33B: AP trampoline yardimcilari (boyut + butunluk + slot yamasi). */
#include "arch/x86_64/longmode.h"

typedef unsigned char u8b;

extern u8b aptramp64_blob[];
extern u8b aptramp64_end[];
extern u8b ap_slot_pml4[];
extern u8b ap_slot_entry[];

u64 aptramp64_size(void) {
    return (u64)(aptramp64_end - aptramp64_blob);
}

int aptramp64_check(const void *blob, u64 size) {
    const u8b *b = (const u8b *)blob;
    u32 magic;
    if (!b || size < 8) return -1;
    magic = (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
    if (magic != 0x36505441UL) return -2; /* "APT6" */
    if (size > 1024) return -3;           /* SIPI sayfasi disi */
    return 0;
}
