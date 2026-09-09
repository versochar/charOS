/* 45B: USB audio — oran listesi + hiz + ses kontrolleri. */
#include "arch/x86_64/longmode.h"

#define USBAUDIO64_MAX_RATES 16
#define USBAUDIO64_MAX_CH 8

static u32 usbaudio64_cur_rate = 48000;
static int usbaudio64_vol[USBAUDIO64_MAX_CH];
static int usbaudio64_muted[USBAUDIO64_MAX_CH];

/* Tip-I format tanimlayici: [0]=boy, [1]=0x24, [2]=0x02(subtype FORMAT),
 * [7]=ornek boyutu, [8..]=ornek hizlari (3 bayt LE her biri). */
int usbaudio64_parse_rates(const unsigned char *desc, int len, u32 *out,
                           int max) {
    int n, i, pos;
    if (!desc || len < 11 || !out || max <= 0) return -1;
    if (desc[1] != 0x24 || desc[2] != 0x02) return -2;
    n = desc[7];
    if (n <= 0 || n > USBAUDIO64_MAX_RATES) return -3;
    if (n > max) n = max;
    pos = 8;
    for (i = 0; i < n; i++) {
        if (pos + 3 > len) return -4;
        out[i] = (u32)desc[pos] | ((u32)desc[pos + 1] << 8) |
                 ((u32)desc[pos + 2] << 16);
        pos += 3;
    }
    return n;
}

int usbaudio64_rate_set(u32 rate) {
    if (!rate || rate > 192000) return -1;
    usbaudio64_cur_rate = rate;
    return 0;
}

u32 usbaudio64_rate_get(void) { return usbaudio64_cur_rate; }

int usbaudio64_volume_set(int ch, int vol_db) {
    if (ch < 0 || ch >= USBAUDIO64_MAX_CH) return -1;
    if (vol_db < -96 || vol_db > 24) return -2;
    usbaudio64_vol[ch] = vol_db;
    return 0;
}

int usbaudio64_volume_get(int ch) {
    if (ch < 0 || ch >= USBAUDIO64_MAX_CH) return -1000;
    return usbaudio64_vol[ch];
}

int usbaudio64_mute(int ch, int on) {
    if (ch < 0 || ch >= USBAUDIO64_MAX_CH) return -1;
    usbaudio64_muted[ch] = on ? 1 : 0;
    return 0;
}
