/* 50A: btrfs skeleton — alt-hacim + salt-okunur anlik goruntu. */
#include "arch/x86_64/longmode.h"

#define BTRFS64_MAX 32

struct btrfs64_subvol {
    int used;
    int readonly;
    char name[48];
    u64 parent;
    u64 id;
};

static struct btrfs64_subvol btrfs64_tab[BTRFS64_MAX];
static u64 btrfs64_next_id = 256; /* 5 altisi varsayilan kok */

static void btrfs_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int btrfs_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct btrfs64_subvol *btrfs_find(const char *name) {
    int i;
    for (i = 0; i < BTRFS64_MAX; i++)
        if (btrfs64_tab[i].used && btrfs_str_eq(btrfs64_tab[i].name, name))
            return &btrfs64_tab[i];
    return 0;
}

int btrfs64_subvol_create(const char *name, u64 parent) {
    int i;
    if (!name) return -1;
    if (btrfs_find(name)) return -2;
    for (i = 0; i < BTRFS64_MAX; i++) {
        if (!btrfs64_tab[i].used) {
            btrfs64_tab[i].used = 1;
            btrfs64_tab[i].readonly = 0;
            btrfs_str_copy(btrfs64_tab[i].name, name, 48);
            btrfs64_tab[i].parent = parent;
            btrfs64_tab[i].id = btrfs64_next_id++;
            return 0;
        }
    }
    return -3;
}

int btrfs64_subvol_delete(const char *name) {
    struct btrfs64_subvol *s = btrfs_find(name);
    int i;
    if (!s) return -1;
    /* Cocugu varsa silinemez */
    for (i = 0; i < BTRFS64_MAX; i++) {
        if (btrfs64_tab[i].used && btrfs64_tab[i].parent == s->id)
            return -2;
    }
    s->used = 0;
    return 0;
}

int btrfs64_snapshot(const char *src, const char *name) {
    struct btrfs64_subvol *s = btrfs_find(src);
    int i;
    if (!s) return -1;
    if (btrfs_find(name)) return -2;
    for (i = 0; i < BTRFS64_MAX; i++) {
        if (!btrfs64_tab[i].used) {
            btrfs64_tab[i].used = 1;
            btrfs64_tab[i].readonly = 1; /* anlik goruntu salt-okunur */
            btrfs_str_copy(btrfs64_tab[i].name, name, 48);
            btrfs64_tab[i].parent = s->id;
            btrfs64_tab[i].id = btrfs64_next_id++;
            return 0;
        }
    }
    return -3;
}

int btrfs64_list(char out[][48], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < BTRFS64_MAX && n < max; i++) {
        int j;
        if (!btrfs64_tab[i].used) continue;
        for (j = 0; btrfs64_tab[i].name[j] && j < 47; j++)
            out[n][j] = btrfs64_tab[i].name[j];
        out[n][j] = 0;
        n++;
    }
    return n;
}
