/* 46G: sample rate conv — dogrusal enterpolasyon, sabit-nokta adim. */
#include "arch/x86_64/longmode.h"

static u32 src64_in_rate = 48000;
static u32 src64_out_rate = 48000;
static u64 src64_step = 65536; /* x65536 */
static u64 src64_pos = 0;
static short src64_last = 0;
static int src64_ready = 0;

int src64_open(u32 in_rate, u32 out_rate) {
    if (!in_rate || !out_rate) return -1;
    src64_in_rate = in_rate;
    src64_out_rate = out_rate;
    src64_step = ((u64)in_rate << 16) / out_rate;
    src64_pos = 0;
    src64_last = 0;
    src64_ready = 1;
    return 0;
}

/* Blok-tabanli: her cagri bagimsiz (sureklilik 47x akista).
 * Cikti adedi = taban((n-1) * out/in). Donus uretilen adet. */
int src64_process(const short *in, int n, short *out, int max) {
    int produced = 0;
    u64 pos = 0;
    if (!src64_ready || !in || n < 2 || !out || max <= 0) return -1;
    while (produced < max) {
        u64 idx = pos >> 16;
        u64 frac = pos & 0xFFFF;
        int v0, v1, v;
        if (idx + 1 >= (u64)n) break; /* giris tukendi */
        v0 = in[idx];
        v1 = in[idx + 1];
        v = v0 + (int)(((long)(v1 - v0) * (long)frac) >> 16);
        out[produced++] = (short)v;
        src64_last = (short)v;
        pos += src64_step;
    }
    src64_pos = pos;
    return produced;
}

u64 src64_ratio(void) { return src64_step; }
