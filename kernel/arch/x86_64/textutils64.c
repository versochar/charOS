/* 54D: find/grep/sed/awk iskeleti (regex64 ustu grep). */
#include "arch/x86_64/longmode.h"

int textutils64_grep(const char *pat, const char *text, char *out,
                     int max) {
    int re, pos = 0, start, o = 0;
    const char *p;
    if (!pat || !text || !out || max <= 0) return -1;
    re = regex64_compile(pat);
    if (re < 0) return -1;
    p = text;
    while (*p) {
        const char *eol = p;
        while (*eol && *eol != '\n') eol++;
        {
            /* Satiri gecici kopyala (skeleton: 256B sinir) */
            char line[256];
            int i, n = 0;
            while (p + n < eol && n < 255) {
                line[n] = p[n];
                n++;
            }
            line[n] = 0;
            if (regex64_search(re, line, &start) == 0) {
                for (i = 0; i < n && o + 1 < max; i++)
                    out[o++] = line[i];
                if (o + 1 < max) out[o++] = '\n';
                pos++;
            }
        }
        p = *eol ? eol + 1 : eol;
    }
    out[o] = 0;
    (void)start;
    return pos; /* eslesen satir sayisi */
}

int textutils64_find(const char *files[], int n, const char *suffix,
                     char out[][64], int max) {
    int i, m = 0, slen = 0;
    if (!files || !suffix || !out || max <= 0) return -1;
    while (suffix[slen]) slen++;
    for (i = 0; i < n && m < max; i++) {
        int flen = 0, k;
        const char *f = files[i];
        if (!f) continue;
        while (f[flen]) flen++;
        if (flen < slen) continue;
        for (k = 0; k < slen; k++) {
            if (f[flen - slen + k] != suffix[k]) break;
        }
        if (k != slen) continue;
        for (k = 0; k < flen && k < 63; k++) out[m][k] = f[k];
        out[m][k] = 0;
        m++;
    }
    return m;
}

int textutils64_sed_replace(const char *text, const char *from,
                            const char *to, char *out, int max) {
    int o = 0, flen = 0, tlen = 0;
    const char *p;
    if (!text || !from || !to || !out || max <= 0) return -1;
    while (from[flen]) flen++;
    while (to[tlen]) tlen++;
    if (!flen) return -1;
    p = text;
    while (*p && o + 1 < max) {
        int i, match = 1;
        for (i = 0; i < flen; i++) {
            if (p[i] != from[i]) {
                match = 0;
                break;
            }
            if (!p[i]) {
                match = 0;
                break;
            }
        }
        if (match) {
            for (i = 0; i < tlen && o + 1 < max; i++)
                out[o++] = to[i];
            p += flen;
        } else {
            out[o++] = *p++;
        }
    }
    out[o] = 0;
    return 0;
}

long long textutils64_awk_sum(const char *text, int col) {
    long long sum = 0;
    const char *p;
    if (!text || col < 0) return 0;
    p = text;
    while (*p) {
        int c = 0;
        /* Sutuna git */
        while (c < col && *p && *p != '\n') {
            while (*p == ' ' || *p == '\t') p++;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            c++;
        }
        while (*p == ' ' || *p == '\t') p++;
        if ((*p >= '0' && *p <= '9') || *p == '-') {
            long long v = 0;
            int neg = 0;
            if (*p == '-') {
                neg = 1;
                p++;
            }
            while (*p >= '0' && *p <= '9') {
                v = v * 10 + (*p - '0');
                p++;
            }
            sum += neg ? -v : v;
        }
        while (*p && *p != '\n') p++;
        if (*p) p++;
    }
    return sum;
}
