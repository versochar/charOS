/* 44A: RTL8821CE probe — PCI esleme + revizyon + BAR denetimi. */
#include "arch/x86_64/longmode.h"

int rtlprobe64_match(u16 vendor, u16 device) {
    return (vendor == RTL8821CE_VENDOR && device == RTL8821CE_DEVICE)
               ? 0
               : -1;
}

/* ID register alt bayti: 0x21=A-kesim, 0x22=B, 0x23=C (skeleton esleme). */
int rtlprobe64_rev(u32 reg_id) {
    u32 cut = reg_id & 0xFF;
    if (cut == 0x21) return 0;
    if (cut == 0x22) return 1;
    if (cut == 0x23) return 2;
    return -1;
}

int rtlprobe64_check_bar(u64 bar, u64 size) {
    if (!bar || !size) return -1;
    if (bar & 0x1) return -2;       /* IO degil, MMIO sart */
    if (bar & 0xFFF) return -3;     /* 4K hizali */
    if (size < 0x4000) return -4;   /* en az 16K pencere */
    if (bar >= 0x100000000ULL) return 1; /* 64-bit BAR (not: tasi) */
    return 0;
}
