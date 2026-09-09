/* 39I: tmpfs — pmm frame zincirli bellek-ici dosyalar.
 * Her dosya: ad + frame listesi + boyut. Veri frame_ptr ile erisilir.
 */
#include "arch/x86_64/longmode.h"

#define TMPFS64_MAX_FILES 32
#define TMPFS64_MAX_FRAMES 64
#define TMPFS64_NAME 48

struct tmpfs64_file {
    int used;
    char name[TMPFS64_NAME];
    u64 frames[TMPFS64_MAX_FRAMES];
    int nframes;
    u64 size;
};

static struct tmpfs64_file tmpfs64_tab[TMPFS64_MAX_FILES];

static void tmpfs_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int tmpfs_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct tmpfs64_file *tmpfs_find(const char *name) {
    int i;
    for (i = 0; i < TMPFS64_MAX_FILES; i++)
        if (tmpfs64_tab[i].used && tmpfs_str_eq(tmpfs64_tab[i].name, name))
            return &tmpfs64_tab[i];
    return 0;
}

int tmpfs64_create(const char *name) {
    int i;
    if (!name || tmpfs_find(name)) return -1;
    for (i = 0; i < TMPFS64_MAX_FILES; i++) {
        if (!tmpfs64_tab[i].used) {
            tmpfs64_tab[i].used = 1;
            tmpfs_str_copy(tmpfs64_tab[i].name, name, TMPFS64_NAME);
            tmpfs64_tab[i].nframes = 0;
            tmpfs64_tab[i].size = 0;
            return 0;
        }
    }
    return -2;
}

/* Gerekli frame'leri buyut (yazma icin). 0=ok. */
static int tmpfs64_grow(struct tmpfs64_file *f, u64 need) {
    while ((u64)f->nframes * PMM64_FRAME < need) {
        u64 frame;
        if (f->nframes >= TMPFS64_MAX_FRAMES) return -1;
        frame = pmm64_alloc_frame();
        if (!frame) return -2;
        f->frames[f->nframes++] = frame;
    }
    return 0;
}

int tmpfs64_write(const char *name, u64 off, const void *buf, u64 len) {
    struct tmpfs64_file *f = tmpfs_find(name);
    const unsigned char *s;
    u64 i;
    if (!f || !buf) return -1;
    if (tmpfs64_grow(f, off + len) != 0) return -2;
    s = (const unsigned char *)buf;
    for (i = 0; i < len; i++) {
        u64 pos = off + i;
        unsigned char *p =
            (unsigned char *)pmm64_frame_ptr(f->frames[pos / PMM64_FRAME]);
        p[pos % PMM64_FRAME] = s[i];
    }
    if (off + len > f->size) f->size = off + len;
    return 0;
}

int tmpfs64_read(const char *name, u64 off, void *buf, u64 len) {
    struct tmpfs64_file *f = tmpfs_find(name);
    unsigned char *d;
    u64 i;
    if (!f || !buf) return -1;
    if (off >= f->size) return 0; /* EOF */
    if (off + len > f->size) len = f->size - off;
    d = (unsigned char *)buf;
    for (i = 0; i < len; i++) {
        u64 pos = off + i;
        unsigned char *p =
            (unsigned char *)pmm64_frame_ptr(f->frames[pos / PMM64_FRAME]);
        d[i] = p[pos % PMM64_FRAME];
    }
    return (int)len;
}

u64 tmpfs64_size(const char *name) {
    struct tmpfs64_file *f = tmpfs_find(name);
    return f ? f->size : 0;
}

int tmpfs64_unlink(const char *name) {
    struct tmpfs64_file *f = tmpfs_find(name);
    int i;
    if (!f) return -1;
    for (i = 0; i < f->nframes; i++) {
        pmm64_free_frame(f->frames[i]);
        f->frames[i] = 0;
    }
    f->used = 0;
    f->nframes = 0;
    f->size = 0;
    return 0;
}
