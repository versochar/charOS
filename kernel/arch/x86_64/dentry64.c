/* 39B: dentry cache — (ebeveyn, ad) -> ino dogrusal tablo. */
#include "arch/x86_64/longmode.h"

#define DENTRY64_MAX 128
#define DENTRY64_NAME 64

struct dentry64_entry {
    int used;
    u64 parent;
    char name[DENTRY64_NAME];
    u64 ino;
};

static struct dentry64_entry dentry64_tab[DENTRY64_MAX];
static int dentry64_n = 0;

static void dentry_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int dentry_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static int dentry_find(u64 parent, const char *name) {
    int i;
    for (i = 0; i < DENTRY64_MAX; i++) {
        if (!dentry64_tab[i].used) continue;
        if (dentry64_tab[i].parent != parent) continue;
        if (dentry_str_eq(dentry64_tab[i].name, name)) return i;
    }
    return -1;
}

int dentry64_insert(u64 parent, const char *name, u64 ino) {
    int i;
    if (!name || !ino) return -1;
    i = dentry_find(parent, name);
    if (i >= 0) {
        dentry64_tab[i].ino = ino; /* guncelle */
        return 0;
    }
    for (i = 0; i < DENTRY64_MAX; i++) {
        if (!dentry64_tab[i].used) {
            dentry64_tab[i].used = 1;
            dentry64_tab[i].parent = parent;
            dentry_str_copy(dentry64_tab[i].name, name, DENTRY64_NAME);
            dentry64_tab[i].ino = ino;
            dentry64_n++;
            return 0;
        }
    }
    return -2; /* dolu */
}

int dentry64_lookup(u64 parent, const char *name, u64 *ino_out) {
    int i;
    if (!name) return -1;
    i = dentry_find(parent, name);
    if (i < 0) return -2; /* miss */
    if (ino_out) *ino_out = dentry64_tab[i].ino;
    return 0;
}

int dentry64_invalidate(u64 parent, const char *name) {
    int i;
    if (!name) return -1;
    i = dentry_find(parent, name);
    if (i < 0) return -2;
    dentry64_tab[i].used = 0;
    dentry64_n--;
    return 0;
}

int dentry64_count(void) { return dentry64_n; }
