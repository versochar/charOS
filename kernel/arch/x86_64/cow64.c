/* 32D: CoW fork cercevesi — paylasim sayaci + kopyala-yaz cozumu.
 * 524288 frame x u16 sayac (1MB). Saf C, asm yok (host testi uygun).
 */
#include "arch/x86_64/longmode.h"

#define COW64_MAX_FRAMES (PMM64_MAX_MEMORY / PMM64_FRAME)

static unsigned short cow64_refs[COW64_MAX_FRAMES];

void cow64_retain(u64 frame) {
    u64 f = frame / PMM64_FRAME;
    if (f >= COW64_MAX_FRAMES) return;
    if (cow64_refs[f] < 0xFFFF) cow64_refs[f]++;
}

int cow64_release(u64 frame) {
    u64 f = frame / PMM64_FRAME;
    if (f >= COW64_MAX_FRAMES) return 0;
    if (cow64_refs[f] > 0) cow64_refs[f]--;
    return (int)cow64_refs[f];
}

int cow64_is_shared(u64 frame) {
    u64 f = frame / PMM64_FRAME;
    if (f >= COW64_MAX_FRAMES) return 0;
    return cow64_refs[f] > 1;
}

int cow64_resolve(u64 frame, u64 *out_new) {
    u64 f = frame / PMM64_FRAME;
    u64 nframe;
    unsigned char *src, *dst;
    u64 i;
    if (f >= COW64_MAX_FRAMES) return -1;
    if (cow64_refs[f] <= 1) { /* paylasim yok, yerinde yazilabilir */
        if (out_new) *out_new = frame;
        return 0;
    }
    nframe = pmm64_alloc_frame();
    if (!nframe) return -2;
    src = (unsigned char *)pmm64_frame_ptr(frame);
    dst = (unsigned char *)pmm64_frame_ptr(nframe);
    for (i = 0; i < PMM64_FRAME; i++) dst[i] = src[i];
    cow64_refs[f]--;
    if (out_new) *out_new = nframe;
    return 0;
}
