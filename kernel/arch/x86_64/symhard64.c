/* 39D: symlink/hardlink — vfsops inode + hedef tablosu. */
#include "arch/x86_64/longmode.h"

#define SYMHARD64_MAX 32
#define SYMHARD64_TARGET 128

struct symhard64_entry {
    int used;
    u64 ino;
    char target[SYMHARD64_TARGET];
};

static struct symhard64_entry symhard64_tab[SYMHARD64_MAX];

static void sym_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int symhard64_symlink(const char *target, u64 *ino_out) {
    int i;
    u64 ino;
    if (!target) return -1;
    if (vfsops64_create(VFS64_SYMLINK, 0777, &ino) != 0) return -2;
    for (i = 0; i < SYMHARD64_MAX; i++) {
        if (!symhard64_tab[i].used) {
            symhard64_tab[i].used = 1;
            symhard64_tab[i].ino = ino;
            sym_str_copy(symhard64_tab[i].target, target, SYMHARD64_TARGET);
            if (ino_out) *ino_out = ino;
            return 0;
        }
    }
    vfsops64_unlink(ino);
    return -3;
}

int symhard64_readlink(u64 ino, char *out, int max) {
    int i, j;
    if (!out || max <= 0) return -1;
    for (i = 0; i < SYMHARD64_MAX; i++) {
        if (!symhard64_tab[i].used || symhard64_tab[i].ino != ino) continue;
        for (j = 0; j + 1 < max && symhard64_tab[i].target[j]; j++)
            out[j] = symhard64_tab[i].target[j];
        out[j] = 0;
        return 0;
    }
    return -2;
}

int symhard64_hardlink(u64 ino) {
    return vfsops64_link(ino); /* kalan nlink */
}
