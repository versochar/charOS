/* 39C / 58C: mount namespace */
#include "arch/x86_64/longmode.h"

#define MN_MAX 64

struct mn_entry {
    int used;
    char path[128];
    char fs[64];
    u64 root_ino;
};

static struct mn_entry mn_tab[MN_MAX];

static void mstr_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int streql(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int mountns64_mount(const char *path, const char *fs, u64 root_ino) {
    int i;
    if (!path || !fs) return -1;
    for (i = 0; i < MN_MAX; i++) {
        if (!mn_tab[i].used) {
            mn_tab[i].used = 1;
            mstr_copy(mn_tab[i].path, path, 128);
            mstr_copy(mn_tab[i].fs, fs, 64);
            mn_tab[i].root_ino = root_ino;
            return 0;
        }
    }
    return -2;
}

int mountns64_unmount(const char *path) {
    int i;
    if (!path) return -1;
    for (i = 0; i < MN_MAX; i++) {
        if (mn_tab[i].used && streql(mn_tab[i].path, path)) {
            mn_tab[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int mountns64_resolve(const char *path, char *inner, int max) {
    int i;
    if (!path || !inner) return -1;
    for (i = 0; i < MN_MAX; i++) {
        if (mn_tab[i].used) {
            /* basit: path'i kopyala */
            mstr_copy(inner, path, max);
            return 0;
        }
    }
    return -1;
}
