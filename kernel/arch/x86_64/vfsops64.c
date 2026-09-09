/* 39A: VFS inode ops — tablo + nlink yasami. */
#include "arch/x86_64/longmode.h"

#define VFSOPS64_MAX 128

struct vfsops64_inode {
    int used;
    u64 ino;
    int type;
    u32 mode;
    u64 size;
    int nlink;
    u32 uid;
    u32 gid;
};

static struct vfsops64_inode vfsops64_tab[VFSOPS64_MAX];
static u64 vfsops64_next = 1;

int vfsops64_create(int type, u32 mode, u64 *ino_out) {
    int i;
    if (type < VFS64_FILE || type > VFS64_SYMLINK) return -1;
    for (i = 0; i < VFSOPS64_MAX; i++) {
        if (!vfsops64_tab[i].used) {
            vfsops64_tab[i].used = 1;
            vfsops64_tab[i].ino = vfsops64_next++;
            vfsops64_tab[i].type = type;
            vfsops64_tab[i].mode = mode;
            vfsops64_tab[i].size = 0;
            vfsops64_tab[i].nlink = 1;
            vfsops64_tab[i].uid = 0;
            vfsops64_tab[i].gid = 0;
            if (ino_out) *ino_out = vfsops64_tab[i].ino;
            return 0;
        }
    }
    return -2;
}

static struct vfsops64_inode *vfsops64_get(u64 ino) {
    int i;
    for (i = 0; i < VFSOPS64_MAX; i++)
        if (vfsops64_tab[i].used && vfsops64_tab[i].ino == ino)
            return &vfsops64_tab[i];
    return 0;
}

int vfsops64_lookup(u64 ino, int *type, u32 *mode, u64 *size) {
    struct vfsops64_inode *in = vfsops64_get(ino);
    if (!in) return -1;
    if (type) *type = in->type;
    if (mode) *mode = in->mode;
    if (size) *size = in->size;
    return 0;
}

int vfsops64_setsize(u64 ino, u64 size) {
    struct vfsops64_inode *in = vfsops64_get(ino);
    if (!in || in->type != VFS64_FILE) return -1;
    in->size = size;
    return 0;
}

int vfsops64_link(u64 ino) {
    struct vfsops64_inode *in = vfsops64_get(ino);
    if (!in) return -1;
    in->nlink++;
    return in->nlink;
}

int vfsops64_nlink(u64 ino) {
    struct vfsops64_inode *in = vfsops64_get(ino);
    if (!in) return -1;
    return in->nlink;
}

int vfsops64_unlink(u64 ino) {
    struct vfsops64_inode *in = vfsops64_get(ino);
    if (!in) return -1;
    if (--in->nlink <= 0) {
        in->used = 0;
        in->nlink = 0;
        return 0; /* serbest kaldi */
    }
    return in->nlink;
}
