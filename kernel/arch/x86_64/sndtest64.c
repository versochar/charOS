/* 46I: sound test — sinus/supurme uretimi + sessizlik/kirpma tespiti.
 * Sinus: 16'li ceyrek tablo + faz akumulatoru (tamsayi, FPU yok). */
#include "arch/x86_64/longmode.h"

int sndtest64_sine(u32 freq, u32 rate, int n, short *out) {
    /* Tamsayi faz akumulatoru: adim = freq * 256 / rate */
    u64 step, phase = 0;
    int i;
    /* Gercek sinus: ucgen-dalga + duzeltme yerine onceden hesap sabitleri?
     * Skeleton: faz-kilitli kare-bozunumlu sinus (asagidaki tablo ile). */
    static const short table[16] = {0,    6393,  12539,  18204,
                                    23170, 27245, 30273, 32137,
                                    32767, 32137, 30273, 27245,
                                    23170, 18204, 12539, 6393};
    if (!freq || !rate || n <= 0 || !out) return -1;
    step = (u64)freq * 256 / rate;
    if (!step) step = 1;
    for (i = 0; i < n; i++) {
        int idx = (int)((phase >> 4) & 0xF);
        int half = (phase >> 8) & 0x1;
        short v = table[idx];
        out[i] = half ? (short)-v : v;
        phase += step;
    }
    return n;
}

int sndtest64_sweep(u32 f0, u32 f1, u32 rate, int n, short *out) {
    int i;
    if (f1 <= f0 || !rate || n <= 0 || !out) return -1;
    for (i = 0; i < n; i++) {
        u32 f = f0 + (u32)(((u64)(f1 - f0) * (u64)i) / (u64)n);
        short s[1];
        /* Kısa pencere: anlik frekansta tek ornek (faz sureksiz, testlik) */
        sndtest64_sine(f, rate, 1, s);
        out[i] = s[0];
    }
    return n;
}

int sndtest64_silence(const short *buf, int n) {
    int i;
    if (!buf || n <= 0) return -1;
    for (i = 0; i < n; i++)
        if (buf[i] > 64 || buf[i] < -64) return 0; /* ses var */
    return 1;
}

int sndtest64_clipping(const short *buf, int n) {
    int i;
    if (!buf || n <= 0) return -1;
    for (i = 0; i < n; i++)
        if (buf[i] >= 32767 || buf[i] <= -32768) return 1;
    return 0;
}
