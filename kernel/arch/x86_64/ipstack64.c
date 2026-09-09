/* 43A: IPv4/IPv6 adresleri — ayristirma/bicim/alt-ag. */
#include "arch/x86_64/longmode.h"

int ipstack64_parse4(const char *s, u32 *out) {
    u32 parts[4] = {0, 0, 0, 0};
    int seg = 0, digits = 0;
    if (!s) return -1;
    for (;;) {
        char c = *s;
        if (c >= '0' && c <= '9') {
            parts[seg] = parts[seg] * 10 + (u32)(c - '0');
            if (parts[seg] > 255) return -2;
            digits++;
        } else if (c == '.' || c == 0) {
            if (!digits) return -3;
            if (c == 0) {
                if (seg != 3) return -5; /* eksik segment */
                break;
            }
            if (seg >= 3) return -3; /* fazla segment */
            seg++;
            digits = 0;
        } else {
            return -4;
        }
        s++;
    }
    if (seg != 3) return -5;
    if (out)
        *out = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) |
               parts[3];
    return 0;
}

int ipstack64_fmt4(u32 ip, char *out, int max) {
    int pos = 0, seg;
    if (!out || max <= 0) return -1;
    for (seg = 0; seg < 4; seg++) {
        unsigned int v = (ip >> (24 - seg * 8)) & 0xFF;
        char tmp[4];
        int n = 0, i;
        if (!v)
            tmp[n++] = '0';
        else {
            char rev[4];
            int rn = 0;
            while (v) {
                rev[rn++] = (char)('0' + (v % 10));
                v /= 10;
            }
            while (rn) tmp[n++] = rev[--rn];
        }
        for (i = 0; i < n; i++) {
            if (pos + 1 >= max) return -2;
            out[pos++] = tmp[i];
        }
        if (seg < 3) {
            if (pos + 1 >= max) return -2;
            out[pos++] = '.';
        }
    }
    out[pos] = 0;
    return 0;
}

/* Tam + "::" kisaltmali IPv6. Cikti 16 bayt ag sirasi. */
int ipstack64_parse6(const char *s, unsigned char out[16]) {
    unsigned short groups[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int head = 0, dbl = -1, i;
    const char *p;
    if (!s || !out) return -1;
    /* "::" konumunu bul */
    p = s;
    while (*p) {
        if (p[0] == ':' && p[1] == ':') {
            if (dbl >= 0) return -2;
            dbl = 1;
        }
        p++;
    }
    p = s;
    i = 0;
    {
        /* Basit yaklasim: once tum gruplari sirayla oku, "::" gorunce
         * kuyruga gec (sondan doldur). Karisik v4-kuyruk 44x'te. */
        int gi = 0;
        int tail_mode = 0;
        unsigned short tgroups[8];
        int tn = 0;
        while (*p) {
            unsigned int v = 0;
            int digits = 0;
            if (p[0] == ':' && p[1] == ':') {
                tail_mode = 1;
                p += 2;
                if (!*p) break;
                continue;
            }
            if (*p == ':') {
                p++;
                continue;
            }
            while ((*p >= '0' && *p <= '9') ||
                   (*p >= 'a' && *p <= 'f') ||
                   (*p >= 'A' && *p <= 'F')) {
                unsigned int d;
                if (*p >= '0' && *p <= '9')
                    d = (unsigned int)(*p - '0');
                else if (*p >= 'a' && *p <= 'f')
                    d = (unsigned int)(*p - 'a' + 10);
                else
                    d = (unsigned int)(*p - 'A' + 10);
                v = v * 16 + d;
                if (v > 0xFFFF) return -3;
                digits++;
                p++;
            }
            if (!digits) return -4;
            if (!tail_mode) {
                if (gi >= 8) return -5;
                groups[gi++] = (unsigned short)v;
            } else {
                if (tn >= 8) return -5;
                tgroups[tn++] = (unsigned short)v;
            }
        }
        if (dbl < 0 && gi != 8) return -6; /* kisaltmasiz 8 grup sart */
        if (dbl >= 0) {
            int zeros = 8 - gi - tn;
            if (zeros < 1 && !(gi == 8 && tn == 0) && tn > 0) {
                /* "::" en az bir sifir acmali (uc/kuyruk bossa apenas) */
                if (gi + tn > 8) return -7;
            }
            if (gi + tn > 8) return -7;
            for (i = 0; i < tn; i++) groups[8 - tn + i] = tgroups[i];
        }
        head = gi;
        (void)head;
    }
    for (i = 0; i < 8; i++) {
        out[i * 2] = (unsigned char)(groups[i] >> 8);
        out[i * 2 + 1] = (unsigned char)(groups[i] & 0xFF);
    }
    return 0;
}

int ipstack64_fmt6(const unsigned char ip[16], char *out, int max) {
    int pos = 0, i;
    if (!ip || !out || max <= 0) return -1;
    for (i = 0; i < 8; i++) {
        unsigned int g = ((unsigned int)ip[i * 2] << 8) | ip[i * 2 + 1];
        char tmp[5];
        int n = 0, k;
        if (!g)
            tmp[n++] = '0';
        else {
            char rev[5];
            int rn = 0;
            while (g) {
                unsigned int d = g % 16;
                rev[rn++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
                g /= 16;
            }
            while (rn) tmp[n++] = rev[--rn];
        }
        for (k = 0; k < n; k++) {
            if (pos + 1 >= max) return -2;
            out[pos++] = tmp[k];
        }
        if (i < 7) {
            if (pos + 1 >= max) return -2;
            out[pos++] = ':';
        }
    }
    out[pos] = 0;
    return 0;
}

int ipstack64_is_v4mapped(const unsigned char ip[16]) {
    int i;
    if (!ip) return 0;
    for (i = 0; i < 10; i++)
        if (ip[i]) return 0;
    if (ip[10] != 0xFF || ip[11] != 0xFF) return 0;
    return 1;
}

int ipstack64_subnet4(u32 ip, u32 net, u32 mask) {
    return (ip & mask) == (net & mask) ? 1 : 0;
}
