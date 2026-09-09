/* 53D: iconv provider — donusum kaydi + yonlendirme. */
#include "arch/x86_64/longmode.h"

#define ICONVPROV64_MAX 16

struct iconvprov64_entry {
    int used;
    char from[32];
    char to[32];
    iconvprov64_fn fn;
};

static struct iconvprov64_entry iconvprov64_tab[ICONVPROV64_MAX];

static void iprov_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int iprov_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        if (!ca) return 1;
    }
}

int iconvprov64_register(const char *from, const char *to,
                         iconvprov64_fn fn) {
    int i;
    if (!from || !to || !fn) return -1;
    for (i = 0; i < ICONVPROV64_MAX; i++) {
        if (!iconvprov64_tab[i].used) {
            iconvprov64_tab[i].used = 1;
            iprov_str_copy(iconvprov64_tab[i].from, from, 32);
            iprov_str_copy(iconvprov64_tab[i].to, to, 32);
            iconvprov64_tab[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

int iconvprov64_convert(const char *from, const char *to,
                        const unsigned char *in, u64 ilen,
                        unsigned char *out, u64 *olen) {
    int i;
    u64 cap;
    if (!from || !to || !in || !out || !olen) return -1;
    cap = *olen;
    for (i = 0; i < ICONVPROV64_MAX; i++) {
        if (!iconvprov64_tab[i].used) continue;
        if (!iprov_str_eq(iconvprov64_tab[i].from, from)) continue;
        if (!iprov_str_eq(iconvprov64_tab[i].to, to)) continue;
        return iconvprov64_tab[i].fn(in, ilen, out, &cap) == 0
                   ? (*olen = cap, 0)
                   : -2;
    }
    return -3; /* saglayici yok */
}
