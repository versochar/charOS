/* 40I: FUSE arayuzu — baglama + islem yonlendirme. */
#include "arch/x86_64/longmode.h"

#define FUSE64_MAX 8

static const struct fuse64_ops *fuse64_tab[FUSE64_MAX];

int fuse64_mount(const struct fuse64_ops *ops) {
    int i;
    if (!ops) return -1;
    for (i = 0; i < FUSE64_MAX; i++) {
        if (!fuse64_tab[i]) {
            fuse64_tab[i] = ops;
            return i;
        }
    }
    return -2;
}

static const struct fuse64_ops *fuse64_get(int h) {
    if (h < 0 || h >= FUSE64_MAX || !fuse64_tab[h]) return 0;
    return fuse64_tab[h];
}

int fuse64_lookup(int h, const char *path, u64 *ino_out) {
    const struct fuse64_ops *o = fuse64_get(h);
    if (!o || !o->lookup) return -1;
    return o->lookup(path, ino_out);
}

int fuse64_read(int h, u64 ino, u64 off, void *buf, u64 len) {
    const struct fuse64_ops *o = fuse64_get(h);
    if (!o || !o->read) return -1;
    return o->read(ino, off, buf, len);
}

int fuse64_write(int h, u64 ino, u64 off, const void *buf, u64 len) {
    const struct fuse64_ops *o = fuse64_get(h);
    if (!o || !o->write) return -1;
    return o->write(ino, off, buf, len);
}

int fuse64_getattr(int h, u64 ino, int *type, u64 *size) {
    const struct fuse64_ops *o = fuse64_get(h);
    if (!o || !o->getattr) return -1;
    return o->getattr(ino, type, size);
}
