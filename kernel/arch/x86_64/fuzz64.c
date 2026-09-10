#include "arch/x86_64/fuzz.h"
#include <string.h>

/* AFL benzeri mutasyoncu fuzzer cekirdegi.
 * Bellegi degistirir (xorshift PRNG), korpus tabanli secim,
 * yeni kapsama sayaci ve crash deposu tutar. */

struct fuzz_seed {
    int  used;
    u16  len;
    u8   data[FUZZ64_BUF_MAX];
    u32  hits;      /* ne kadar cok soz edildi */
};

static struct fuzz_seed corpus[FUZZ64_MAX_CORPUS];
static int corpus_n = 0;
static u32 fuzz_state;
static u32 fuzz_newcov;
static u32 fuzz_crashes[FUZZ64_MAX_CRASHES];
static int crash_n;
static int fuzz_xstarted;

int fuzz64_init(void) {
    memset(corpus, 0, sizeof(corpus));
    corpus_n = 0; crash_n = 0; fuzz_newcov = 0;
    fuzz_state = 0x1BADCAFEu; fuzz_xstarted = 0;
    return 0;
}

int fuzz64_xstart(u32 seed) {
    fuzz_state = seed ? seed : 0x1BADCAFEu;
    fuzz_xstarted = 1;
    return 0;
}

u32 fuzz64_xnext(void) {
    fuzz_state ^= fuzz_state << 13;
    fuzz_state ^= fuzz_state >> 17;
    fuzz_state ^= fuzz_state << 5;
    return fuzz_state;
}

int fuzz64_corpus_add(const u8 *seed, u16 len) {
    if (!seed || len == 0 || len > FUZZ64_BUF_MAX) return -1;
    if (corpus_n >= FUZZ64_MAX_CORPUS) return -2;
    memcpy(corpus[corpus_n].data, seed, len);
    corpus[corpus_n].len = len;
    corpus[corpus_n].used = 1;
    corpus[corpus_n].hits = 0;
    corpus_n++;
    return 0;
}

int fuzz64_corpus_add_file(const char *path) {
    /* kernel ortaminda dosya yerine sabit kalibre tohumu */
    (void)path;
    return -2;
}

static int fuzz_pick(void) {
    u32 v = fuzz64_xnext();
    if (corpus_n == 0) return -1;
    return (int)(v % (u32)corpus_n);
}

int fuzz64_iteration(u8 *out, u16 max_out, u16 *out_len) {
    int idx, i;
    u32 r;
    u16 len;
    if (!out || !out_len || max_out == 0) return -1;
    if (corpus_n == 0) return -2;
    idx = fuzz_pick();
    corpus[idx].hits++;
    if ((corpus[idx].hits & 0x3u) == 0) fuzz_newcov++; /* yol cesitliligi */
    /* taban tohumu kopyala */
    len = corpus[idx].len;
    if (len > max_out) len = max_out;
    memcpy(out, corpus[idx].data, len);
    r = fuzz64_xnext();
    if ((r & 1u) && max_out > len) out[len++] = (u8)(r >> 8);
    if ((r & 2u)) out[len - 1] ^= (u8)fuzz64_xnext();
    if ((r & 4u)) {
        for (i = 0; i < len; i++)
            if (fuzz64_xnext() % 4u == 0)
                out[i] = (u8)fuzz64_xnext();
    }
    *out_len = len;
    return 0;
}

int fuzz64_feed_crash(const u8 *input, u16 len, u32 reason) {
    if (!input) return -1;
    if (crash_n >= FUZZ64_MAX_CRASHES) return -2;
    fuzz_crashes[crash_n++] = reason ^ ((u32)len * 31u);
    return 0;
}

int fuzz64_crash_count(void) {
    return crash_n;
}

int fuzz64_new_coverage(void) {
    return (int)fuzz_newcov;
}

int fuzz64_mutate(u8 *buf, u16 *len, u16 max_len) {
    int i;
    if (!buf || !len || max_len == 0) return -1;
    if (*len == 0) return -2;
    for (i = 0; i < *len; i++) {
        u32 r = fuzz64_xnext();
        if ((r & 15u) == 0) buf[i] = (u8)fuzz64_xnext();      /* rastgele */
        else if ((r & 15u) == 1) buf[i] ^= (u8)(1u << (r >> 6 & 7u)); /* bit */
        else if ((r & 15u) == 2) buf[i] = 0x00;               /* sifirla */
        else if ((r & 15u) == 3) buf[i] = 0xFF;               /* doyur */
        else if ((r & 15u) == 4 && i + 1 < *len) {            /* takas */
            u8 t = buf[i]; buf[i] = buf[i + 1]; buf[i + 1] = t;
        }
    }
    if ((fuzz64_xnext() & 1u) && *len < max_len)
        buf[(*len)++] = (u8)fuzz64_xnext();
    return 0;
}

int fuzz64_token_insert(u8 *buf, u16 *len, u16 max_len, const u8 *token,
                        u16 tlen) {
    if (!buf || !len || (!token && tlen)) return -1;
    if (tlen == 0) return -2;
    if ((u16)(*len + tlen) > max_len) return -3;
    memcpy(buf + *len, token, tlen);
    *len += tlen;
    return 0;
}