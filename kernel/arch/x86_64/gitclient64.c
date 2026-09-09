/* 54F: git client — GERCEK SHA-1 + nesne + ref cozumu. */
#include "arch/x86_64/longmode.h"

static u32 git_rol(u32 v, int n) { return (v << n) | (v >> (32 - n)); }

/* Tek 64B blok sikistirma (FIPS 180-4). */
static void git_sha1_block(const unsigned char *blk, u32 h[5]) {
    u32 w[80];
    u32 a, b, c, d, e, f, k, t;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((u32)blk[i * 4] << 24) | ((u32)blk[i * 4 + 1] << 16) |
               ((u32)blk[i * 4 + 2] << 8) | blk[i * 4 + 3];
    for (i = 16; i < 80; i++)
        w[i] = git_rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    a = h[0];
    b = h[1];
    c = h[2];
    d = h[3];
    e = h[4];
    for (i = 0; i < 80; i++) {
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999UL;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1UL;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCUL;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6UL;
        }
        t = git_rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = git_rol(b, 30);
        b = a;
        a = t;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

/* Standart SHA-1: tam bloklar + acik dolgu blogu (1-2 blok). */
int gitclient64_sha1(const void *data, u64 len, unsigned char out[20]) {
    const unsigned char *p = (const unsigned char *)data;
    u64 bitlen = len * 8;
    u32 h[5] = {0x67452301UL, 0xEFCDAB89UL, 0x98BADCFEUL, 0x10325476UL,
                0xC3D2E1F0UL};
    u64 off = 0, rem, i;
    int t;
    unsigned char tail[128];
    if (!p && len) return -1;
    if (!out) return -1;
    while (off + 64 <= len) {
        git_sha1_block(p + off, h);
        off += 64;
    }
    rem = len - off;
    for (i = 0; i < 128; i++) tail[i] = 0;
    for (i = 0; i < rem; i++) tail[i] = p[off + i];
    tail[rem] = 0x80;
    if (rem + 1 + 8 <= 64) {
        for (i = 0; i < 8; i++)
            tail[56 + i] = (unsigned char)((bitlen >> (56 - i * 8)) & 0xFF);
        git_sha1_block(tail, h);
    } else {
        for (i = 0; i < 8; i++)
            tail[120 + i] = (unsigned char)((bitlen >> (56 - i * 8)) & 0xFF);
        git_sha1_block(tail, h);
        git_sha1_block(tail + 64, h);
    }
    for (t = 0; t < 5; t++) {
        out[t * 4] = (unsigned char)((h[t] >> 24) & 0xFF);
        out[t * 4 + 1] = (unsigned char)((h[t] >> 16) & 0xFF);
        out[t * 4 + 2] = (unsigned char)((h[t] >> 8) & 0xFF);
        out[t * 4 + 3] = (unsigned char)(h[t] & 0xFF);
    }
    return 0;
}

/* Nesne: "tur uzunluk\0" + veri; donus toplam boy. */
int gitclient64_object(const char *type, const void *data, u64 len,
                       unsigned char *out, int max) {
    u64 tlen = 0, hdr, i;
    const unsigned char *p;
    if (!type || !out || max <= 0) return -1;
    if (!data) len = 0;
    while (type[tlen]) tlen++;
    {
        u64 digits = 1, tmp = len;
        while (tmp >= 10) {
            digits++;
            tmp /= 10;
        }
        hdr = tlen + 1 + digits + 1;
        if (hdr + len > (u64)max) return -2;
        for (i = 0; i < tlen; i++) out[i] = (unsigned char)type[i];
        out[tlen] = ' ';
        /* Uzunluk ondalik */
        for (i = 0; i < digits; i++) {
            u64 div = 1, k;
            for (k = 0; k + 1 < digits - i; k++) div *= 10;
            out[tlen + 1 + i] = (unsigned char)('0' + (len / div) % 10);
        }
        out[tlen + 1 + digits] = 0;
    }
    p = (const unsigned char *)data;
    for (i = 0; i < len; i++) out[hdr + i] = p[i];
    return (int)(hdr + len);
}

/* "hash SP ad\n" satiri coz. */
int gitclient64_ref(const char *line, char *hash_out, char *name_out) {
    int i = 0, j;
    if (!line) return -1;
    while (line[i] && line[i] != ' ' && i < 40) {
        if (hash_out) hash_out[i] = line[i];
        i++;
    }
    if (i != 40 || line[i] != ' ') return -2;
    if (hash_out) hash_out[40] = 0;
    i++;
    j = 0;
    while (line[i] && line[i] != '\n' && j < 127) {
        if (name_out) name_out[j] = line[i];
        i++;
        j++;
    }
    if (name_out) name_out[j] = 0;
    if (!j) return -3;
    return 0;
}
