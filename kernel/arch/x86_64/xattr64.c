/* 40D: xattr — inode basina ad/deger tablosu. */
#include "arch/x86_64/longmode.h"

#define XATTR64_MAX 64
#define XATTR64_NAME 32
#define XATTR64_VALUE 128

struct xattr64_entry {
    int used;
    u64 ino;
    char name[XATTR64_NAME];
    unsigned char value[XATTR64_VALUE];
    u64 len;
};

static struct xattr64_entry xattr64_tab[XATTR64_MAX];

static void xattr_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int xattr_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static int xattr_find(u64 ino, const char *name) {
    int i;
    for (i = 0; i < XATTR64_MAX; i++) {
        if (!xattr64_tab[i].used) continue;
        if (xattr64_tab[i].ino != ino) continue;
        if (xattr_str_eq(xattr64_tab[i].name, name)) return i;
    }
    return -1;
}

int xattr64_set(u64 ino, const char *name, const void *val, u64 len) {
    const unsigned char *s;
    u64 i;
    int idx;
    if (!ino || !name || (!val && len)) return -1;
    if (len > XATTR64_VALUE) return -2;
    idx = xattr_find(ino, name);
    if (idx < 0) {
        for (idx = 0; idx < XATTR64_MAX; idx++) {
            if (!xattr64_tab[idx].used) break;
        }
        if (idx >= XATTR64_MAX) return -3;
        xattr64_tab[idx].used = 1;
        xattr64_tab[idx].ino = ino;
        xattr_str_copy(xattr64_tab[idx].name, name, XATTR64_NAME);
    }
    xattr64_tab[idx].len = len;
    s = (const unsigned char *)val;
    for (i = 0; i < len; i++) xattr64_tab[idx].value[i] = s[i];
    return 0;
}

int xattr64_get(u64 ino, const char *name, void *out, u64 *len) {
    int idx;
    u64 i;
    unsigned char *d;
    if (!ino || !name) return -1;
    idx = xattr_find(ino, name);
    if (idx < 0) return -2;
    if (len) {
        if (out && *len < xattr64_tab[idx].len) {
            *len = xattr64_tab[idx].len;
            return -3; /* tampon kucuk */
        }
        *len = xattr64_tab[idx].len;
    }
    if (out) {
        d = (unsigned char *)out;
        for (i = 0; i < xattr64_tab[idx].len; i++)
            d[i] = xattr64_tab[idx].value[i];
    }
    return 0;
}

int xattr64_list(u64 ino, char *out, int max) {
    int i, pos = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < XATTR64_MAX; i++) {
        int j;
        if (!xattr64_tab[i].used || xattr64_tab[i].ino != ino) continue;
        for (j = 0; xattr64_tab[i].name[j]; j++) {
            if (pos + 1 >= max) return -2;
            out[pos++] = xattr64_tab[i].name[j];
        }
        if (pos + 1 >= max) return -2;
        out[pos++] = 0;
    }
    if (pos < max)
        out[pos] = 0; /* cift NUL son */
    return pos;
}

int xattr64_remove(u64 ino, const char *name) {
    int idx;
    if (!ino || !name) return -1;
    idx = xattr_find(ino, name);
    if (idx < 0) return -2;
    xattr64_tab[idx].used = 0;
    return 0;
}
