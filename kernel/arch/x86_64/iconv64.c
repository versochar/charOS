/* 51G: iconv — UTF-8 <-> UTF-16/UTF-32/Latin-1 donusumu. */
#include "arch/x86_64/longmode.h"

#define ICONV64_MAX 8
#define ICONV64_UTF8 0
#define ICONV64_UTF16 1
#define ICONV64_UTF32 2
#define ICONV64_LATIN1 3

struct iconv64_cd {
    int used;
    int to;
    int from;
};

static struct iconv64_cd iconv64_tab[ICONV64_MAX];

static int iconv_enc_id(const char *s) {
    if (!s) return -1;
    if ((s[0] == 'U' || s[0] == 'u') && s[1] == 'T' &&
        (s[2] == 'F' || s[2] == 'f') && s[3] == '-') {
        if (s[4] == '8' && !s[5]) return ICONV64_UTF8;
        if (s[4] == '1' && s[5] == '6' && !s[6]) return ICONV64_UTF16;
        if (s[4] == '3' && s[5] == '2' && !s[6]) return ICONV64_UTF32;
    }
    if ((s[0] == 'L' || s[0] == 'l') && s[1] == 'A' &&
        (s[2] == 'T' || s[2] == 't'))
        return ICONV64_LATIN1;
    if (s[0] == 'I' && s[1] == 'S' && s[2] == 'O' && s[3] == '-' &&
        s[4] == '8' && s[5] == '8' && s[6] == '5' && s[7] == '9' && !s[8])
        return ICONV64_LATIN1;
    return -1;
}

int iconv64_open(const char *to, const char *from) {
    int i, t, f;
    t = iconv_enc_id(to);
    f = iconv_enc_id(from);
    if (t < 0 || f < 0) return -1;
    for (i = 0; i < ICONV64_MAX; i++) {
        if (!iconv64_tab[i].used) {
            iconv64_tab[i].used = 1;
            iconv64_tab[i].to = t;
            iconv64_tab[i].from = f;
            return i;
        }
    }
    return -2;
}

/* Bir Unicode noktasi coz (UTF-8/Latin-1 giris); donus tuketilen bayt. */
static int iconv_decode(int from, const unsigned char *p, u64 left,
                        u32 *cp) {
    if (from == ICONV64_LATIN1) {
        if (!left) return 0;
        *cp = p[0];
        return 1;
    }
    if (from == ICONV64_UTF8) {
        if (!left) return 0;
        if (p[0] < 0x80) {
            *cp = p[0];
            return 1;
        }
        if ((p[0] & 0xE0) == 0xC0) {
            if (left < 2 || (p[1] & 0xC0) != 0x80) return -1;
            *cp = ((u32)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            if (*cp < 0x80) return -1; /* asiri-uzun */
            return 2;
        }
        if ((p[0] & 0xF0) == 0xE0) {
            if (left < 3 || (p[1] & 0xC0) != 0x80 ||
                (p[2] & 0xC0) != 0x80)
                return -1;
            *cp = ((u32)(p[0] & 0x0F) << 12) |
                  ((u32)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            if (*cp < 0x800) return -1;
            return 3;
        }
        if ((p[0] & 0xF8) == 0xF0) {
            if (left < 4) return -1;
            *cp = ((u32)(p[0] & 0x07) << 18) |
                  ((u32)(p[1] & 0x3F) << 12) |
                  ((u32)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            if (*cp < 0x10000 || *cp > 0x10FFFF) return -1;
            return 4;
        }
        return -1;
    }
    if (from == ICONV64_UTF16) {
        u32 lo, hi;
        if (left < 2) return 0;
        lo = p[0] | ((u32)p[1] << 8);
        if (lo < 0xD800 || lo > 0xDFFF) {
            *cp = lo;
            return 2;
        }
        if (left < 4) return -1;
        hi = p[2] | ((u32)p[3] << 8);
        if (lo > 0xDBFF || hi < 0xDC00 || hi > 0xDFFF) return -1;
        *cp = 0x10000 + ((lo - 0xD800) << 10) + (hi - 0xDC00);
        return 4;
    }
    if (from == ICONV64_UTF32) {
        u32 v;
        int i;
        if (left < 4) return 0;
        v = 0;
        for (i = 0; i < 4; i++) v |= (u32)p[i] << (i * 8);
        if (v > 0x10FFFF) return -1;
        *cp = v;
        return 4;
    }
    return -1;
}

/* Unicode noktasi yaz; donus yazilan bayt (<0 sigmaz). */
static int iconv_encode(int to, u32 cp, unsigned char *o, u64 left) {
    if (to == ICONV64_LATIN1) {
        if (!left) return -1;
        if (cp > 0xFF) return -2; /* temsil disi */
        o[0] = (unsigned char)cp;
        return 1;
    }
    if (to == ICONV64_UTF8) {
        if (cp < 0x80) {
            if (!left) return -1;
            o[0] = (unsigned char)cp;
            return 1;
        }
        if (cp < 0x800) {
            if (left < 2) return -1;
            o[0] = (unsigned char)(0xC0 | (cp >> 6));
            o[1] = (unsigned char)(0x80 | (cp & 0x3F));
            return 2;
        }
        if (cp < 0x10000) {
            if (left < 3) return -1;
            o[0] = (unsigned char)(0xE0 | (cp >> 12));
            o[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
            o[2] = (unsigned char)(0x80 | (cp & 0x3F));
            return 3;
        }
        if (left < 4) return -1;
        o[0] = (unsigned char)(0xF0 | (cp >> 18));
        o[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
        o[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
        o[3] = (unsigned char)(0x80 | (cp & 0x3F));
        return 4;
    }
    if (to == ICONV64_UTF16) {
        if (cp < 0x10000) {
            if (left < 2) return -1;
            if (cp >= 0xD800 && cp <= 0xDFFF) return -2;
            o[0] = (unsigned char)(cp & 0xFF);
            o[1] = (unsigned char)(cp >> 8);
            return 2;
        }
        if (left < 4) return -1;
        cp -= 0x10000;
        o[0] = (unsigned char)(((cp >> 10) + 0xD800) & 0xFF);
        o[1] = (unsigned char)(((cp >> 10) + 0xD800) >> 8);
        o[2] = (unsigned char)(((cp & 0x3FF) + 0xDC00) & 0xFF);
        o[3] = (unsigned char)(((cp & 0x3FF) + 0xDC00) >> 8);
        return 4;
    }
    if (to == ICONV64_UTF32) {
        int i;
        if (left < 4) return -1;
        for (i = 0; i < 4; i++) o[i] = (unsigned char)((cp >> (i * 8)) & 0xFF);
        return 4;
    }
    return -1;
}

int iconv64_convert(int cd, const unsigned char **in, u64 *inleft,
                    unsigned char **out, u64 *outleft) {
    struct iconv64_cd *c;
    if (cd < 0 || cd >= ICONV64_MAX || !iconv64_tab[cd].used) return -1;
    if (!in || !*in || !inleft || !out || !*out || !outleft) return -1;
    c = &iconv64_tab[cd];
    while (*inleft > 0) {
        u32 cp = 0;
        int used, wrote;
        used = iconv_decode(c->from, *in, *inleft, &cp);
        if (used <= 0) return -2; /* EILSEQ / eksik */
        wrote = iconv_encode(c->to, cp, *out, *outleft);
        if (wrote < 0) return -3; /* E2BIG / temsil */
        *in += used;
        *inleft -= (u64)used;
        *out += wrote;
        *outleft -= (u64)wrote;
    }
    return 0;
}

int iconv64_close(int cd) {
    if (cd < 0 || cd >= ICONV64_MAX || !iconv64_tab[cd].used) return -1;
    iconv64_tab[cd].used = 0;
    return 0;
}
