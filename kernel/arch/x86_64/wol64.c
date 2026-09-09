/* 48I: WoL — sihirli paket kurma + desen filtreleri + esleme. */
#include "arch/x86_64/longmode.h"

#define WOL64_MAX_PATTERNS 8
#define WOL64_PATLEN 16

struct wol64_pattern {
    int used;
    u32 off;
    unsigned char mask[WOL64_PATLEN];
    unsigned char pat[WOL64_PATLEN];
    int len;
};

static struct wol64_pattern wol64_tab[WOL64_MAX_PATTERNS];

/* 6xFF + 16xMAC = 102 bayt; donus boy. */
int wol64_magic(const unsigned char mac[6], unsigned char *out) {
    int i, j;
    if (!mac || !out) return -1;
    for (i = 0; i < 6; i++) out[i] = 0xFF;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 6; j++) out[6 + i * 6 + j] = mac[j];
    }
    return 102;
}

int wol64_add_pattern(u32 off, const unsigned char *mask,
                      const unsigned char *pat, int len) {
    int i, k;
    if (!mask || !pat || len <= 0 || len > WOL64_PATLEN) return -1;
    for (i = 0; i < WOL64_MAX_PATTERNS; i++) {
        if (!wol64_tab[i].used) {
            wol64_tab[i].used = 1;
            wol64_tab[i].off = off;
            for (k = 0; k < len; k++) {
                wol64_tab[i].mask[k] = mask[k];
                wol64_tab[i].pat[k] = pat[k];
            }
            wol64_tab[i].len = len;
            return 0;
        }
    }
    return -2;
}

/* 1=uyandir (sihirli paket veya desen), 0=uygun degil. */
int wol64_match(const unsigned char *frame, int len) {
    int i, k;
    if (!frame || len < 102) {
        /* Kisa cerceve: yalniz desen dene */
        if (!frame || len <= 0) return 0;
    } else {
        int magic = 1;
        for (i = 0; i < 6; i++) {
            if (frame[i] != 0xFF) {
                magic = 0;
                break;
            }
        }
        if (magic) {
            /* 16 tekrar ayni MAC mi? */
            int same = 1;
            for (i = 1; i < 16 && same; i++) {
                for (k = 0; k < 6; k++) {
                    if (frame[6 + i * 6 + k] != frame[6 + k]) {
                        same = 0;
                        break;
                    }
                }
            }
            if (same) return 1;
        }
    }
    for (i = 0; i < WOL64_MAX_PATTERNS; i++) {
        int hit = 1;
        if (!wol64_tab[i].used) continue;
        if ((u64)wol64_tab[i].off + (u64)wol64_tab[i].len > (u64)len)
            continue;
        for (k = 0; k < wol64_tab[i].len; k++) {
            if ((frame[wol64_tab[i].off + k] & wol64_tab[i].mask[k]) !=
                (wol64_tab[i].pat[k] & wol64_tab[i].mask[k])) {
                hit = 0;
                break;
            }
        }
        if (hit) return 1;
    }
    return 0;
}
