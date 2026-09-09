/* 32A: PMM64 — 2GB cap bitmap, 4K frame.
 * 524288 frame -> 8192 u64 bitmap (64KB). Saf C, asm yok (host testi uygun).
 */
#include "arch/x86_64/longmode.h"

#define PMM64_MAX_FRAMES (PMM64_MAX_MEMORY / PMM64_FRAME)
#define PMM64_BITMAP_WORDS (PMM64_MAX_FRAMES / 64)

static u64 pmm64_bitmap[PMM64_BITMAP_WORDS];
static u64 pmm64_total = 0;
static u64 pmm64_free = 0;
static int pmm64_ready = 0;

static void *pmm64_identity(u64 frame) { return (void *)(frame); }
static void *(*pmm64_mapper)(u64 frame) = pmm64_identity;

void *pmm64_frame_ptr(u64 frame) { return pmm64_mapper(frame); }
void pmm64_set_frame_mapper(void *(*fn)(u64 frame)) {
    pmm64_mapper = fn ? fn : pmm64_identity;
}

static void bit_set(u64 f) { pmm64_bitmap[f >> 6] |= (1ULL << (f & 63)); }
static void bit_clear(u64 f) { pmm64_bitmap[f >> 6] &= ~(1ULL << (f & 63)); }
static int bit_get(u64 f) { return (int)((pmm64_bitmap[f >> 6] >> (f & 63)) & 1ULL); }

void pmm64_init(void) {
    for (u64 i = 0; i < PMM64_BITMAP_WORDS; i++) pmm64_bitmap[i] = ~0ULL;
    pmm64_total = 0;
    pmm64_free = 0;
    pmm64_ready = 1;
}

/* NOT: frame 0 (NULL sayfa) asla dagitilmaz; adres 0 = gecersiz sentinel.
 * Klasik OS konvansiyonu: NULL pointer her zaman hatalidir. */
void pmm64_add_region(u64 base, u64 len) {
    if (!pmm64_ready) pmm64_init();
    u64 start = (base + PMM64_FRAME - 1) / PMM64_FRAME;
    u64 end = (base + len) / PMM64_FRAME;
    if (end > PMM64_MAX_FRAMES) end = PMM64_MAX_FRAMES;
    if (start < 1) start = 1;
    for (u64 f = start; f < end; f++) {
        if (bit_get(f)) { bit_clear(f); pmm64_total++; pmm64_free++; }
    }
}

void pmm64_reserve(u64 base, u64 len) {
    u64 start, end, f;
    if (!pmm64_ready) return;
    start = base / PMM64_FRAME;
    end = (base + len + PMM64_FRAME - 1) / PMM64_FRAME;
    if (end > PMM64_MAX_FRAMES) end = PMM64_MAX_FRAMES;
    for (f = start; f < end; f++) {
        if (!bit_get(f)) { bit_set(f); pmm64_free--; }
    }
}

u64 pmm64_alloc_frame(void) {
    u64 i, f;
    int b;
    if (!pmm64_ready || !pmm64_free) return 0;
    for (i = 0; i < PMM64_BITMAP_WORDS; i++) {
        u64 w = pmm64_bitmap[i];
        if (w == ~0ULL) continue;
        for (b = 0; b < 64; b++) {
            if (!(w & (1ULL << b))) {
                f = (i << 6) + (u64)b;
                if (f >= PMM64_MAX_FRAMES) return 0;
                bit_set(f);
                pmm64_free--;
                return f * PMM64_FRAME;
            }
        }
    }
    return 0;
}

void pmm64_free_frame(u64 frame) {
    u64 f = frame / PMM64_FRAME;
    if (!pmm64_ready || f >= PMM64_MAX_FRAMES) return;
    if (bit_get(f)) { bit_clear(f); pmm64_free++; }
}

u64 pmm64_free_frames(void) { return pmm64_free; }
u64 pmm64_total_frames(void) { return pmm64_total; }

/* 35A destegi: [lo,hi) frame araligindan ilk bos frame. */
u64 pmm64_alloc_range(u64 lo_frame, u64 hi_frame) {
    u64 f;
    if (!pmm64_ready || !pmm64_free) return 0;
    if (lo_frame >= hi_frame || lo_frame >= PMM64_MAX_FRAMES) return 0;
    if (hi_frame > PMM64_MAX_FRAMES) hi_frame = PMM64_MAX_FRAMES;
    for (f = lo_frame; f < hi_frame; f++) {
        if (!bit_get(f)) {
            bit_set(f);
            pmm64_free--;
            return f * PMM64_FRAME;
        }
    }
    return 0;
}
