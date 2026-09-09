/* 34D: GOP 64-bit framebuffer — mod dogrulama + esleme hesabi. */
#include "arch/x86_64/longmode.h"

int gop64_validate(u32 w, u32 h, u32 pitch_px, u32 format) {
    if (!w || !h || !pitch_px) return -1;
    if (w > 4096 || h > 4096) return -2;
    if (pitch_px < w) return -3;
    if (pitch_px > 8192) return -4;
    if (format > 3) return -5; /* 0=RGB,1=BGR,2=bitmask,3=blt */
    return 0;
}

u64 gop64_size_bytes(u32 h, u32 pitch_px) {
    return (u64)h * (u64)pitch_px * 4ULL;
}

u64 gop64_pages(u32 h, u32 pitch_px) {
    u64 sz = gop64_size_bytes(h, pitch_px);
    return (sz + 0xFFFULL) / 0x1000ULL;
}

u32 gop64_fill_color(u32 rgb, u32 format) {
    u32 r = (rgb >> 16) & 0xFF;
    u32 g = (rgb >> 8) & 0xFF;
    u32 b = rgb & 0xFF;
    if (format == 1) return (b << 16) | (g << 8) | r; /* BGR takas */
    return rgb;
}
