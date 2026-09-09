/* 52C: dynamic linker tablolari — girdi okuma + NEEDED/STR listesi. */
#include "arch/x86_64/longmode.h"

struct dynlink64_ent {
    long tag;
    u64 val;
};

#define DT_NEEDED64 1
#define DT_SONAME64 14
#define DT_RPATH64 15
#define DT_RUNPATH64 29

static void dyn_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int dynlink64_get(const void *dyn, int nent, long tag, u64 *val_out) {
    const struct dynlink64_ent *e = (const struct dynlink64_ent *)dyn;
    int i;
    if (!e || nent <= 0) return -1;
    for (i = 0; i < nent; i++) {
        if (e[i].tag == tag) {
            if (val_out) *val_out = e[i].val;
            return 0;
        }
        if (e[i].tag == 0) break; /* DT_NULL */
    }
    return -2;
}

int dynlink64_needed(const void *dyn, int nent, const char *strtab,
                     u64 strsz, char out[][32], int max) {
    const struct dynlink64_ent *e = (const struct dynlink64_ent *)dyn;
    int i, n = 0;
    if (!e || nent <= 0 || !strtab || !out || max <= 0) return -1;
    for (i = 0; i < nent && n < max; i++) {
        u64 off;
        int k;
        if (e[i].tag == 0) break;
        if (e[i].tag != DT_NEEDED64) continue;
        off = e[i].val;
        if (off >= strsz) return -2;
        for (k = 0; strtab[off + k] && k < 31; k++) {
            if (off + (u64)k >= strsz) return -3;
            out[n][k] = strtab[off + k];
        }
        out[n][k] = 0;
        n++;
    }
    return n;
}

int dynlink64_str(const void *dyn, int nent, long tag, const char *strtab,
                  u64 strsz, char *out, int max) {
    u64 off;
    int k;
    if (!strtab || !out || max <= 0) return -1;
    if (dynlink64_get(dyn, nent, tag, &off) != 0) return -2;
    if (off >= strsz) return -3;
    dyn_str_copy(out, strtab + off, max);
    return 0;
}
