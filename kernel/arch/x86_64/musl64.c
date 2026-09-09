/* 51A: musl cekirdek ilkeller — ortusme-guvenli bellek + dizgi. */
#include "arch/x86_64/longmode.h"

void *musl64_memcpy(void *d, const void *s, u64 n) {
    unsigned char *dd = (unsigned char *)d;
    const unsigned char *ss = (const unsigned char *)s;
    u64 i;
    if (!dd || !ss) return d;
    for (i = 0; i < n; i++) dd[i] = ss[i];
    return d;
}

void *musl64_memmove(void *d, const void *s, u64 n) {
    unsigned char *dd = (unsigned char *)d;
    const unsigned char *ss = (const unsigned char *)s;
    u64 i;
    if (!dd || !ss || !n) return d;
    if (dd < ss) {
        for (i = 0; i < n; i++) dd[i] = ss[i];
    } else if (dd > ss) {
        i = n;
        while (i--) dd[i] = ss[i];
    }
    return d;
}

void *musl64_memset(void *d, int c, u64 n) {
    unsigned char *dd = (unsigned char *)d;
    u64 i;
    if (!dd) return d;
    for (i = 0; i < n; i++) dd[i] = (unsigned char)c;
    return d;
}

u64 musl64_strlen(const char *s) {
    u64 n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

int musl64_strcmp(const char *a, const char *b) {
    u64 i = 0;
    if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
    for (;; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return ca < cb ? -1 : 1;
        if (!ca) return 0;
    }
}

int musl64_version(char *out, int max) {
    const char *v = "charos-libc-0.1.1-musl-compat";
    int i;
    if (!out || max <= 0) return -1;
    for (i = 0; v[i] && i + 1 < max; i++) out[i] = v[i];
    out[i] = 0;
    return 0;
}
