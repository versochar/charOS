/* 51D: stdio buffering — kip bazli tampon + yikama + konum. */
#include "arch/x86_64/longmode.h"

#define STDIO64_MAX 16
#define STDIO64_BUF 1024

struct stdio64_file {
    int used;
    int mode; /* NONE/LINE/FULL */
    unsigned char buf[STDIO64_BUF];
    u64 len;
    u64 pos; /* okuma imleci */
    u64 flushed;
};

static struct stdio64_file stdio64_tab[STDIO64_MAX];

int stdio64_open(int mode) {
    int i;
    if (mode < STDIO64_NONE || mode > STDIO64_FULL) return -1;
    for (i = 0; i < STDIO64_MAX; i++) {
        if (!stdio64_tab[i].used) {
            stdio64_tab[i].used = 1;
            stdio64_tab[i].mode = mode;
            stdio64_tab[i].len = 0;
            stdio64_tab[i].pos = 0;
            stdio64_tab[i].flushed = 0;
            return i;
        }
    }
    return -2;
}

static struct stdio64_file *stdio64_get(int f) {
    if (f < 0 || f >= STDIO64_MAX || !stdio64_tab[f].used) return 0;
    return &stdio64_tab[f];
}

int stdio64_flush(int f) {
    struct stdio64_file *s = stdio64_get(f);
    if (!s) return -1;
    s->flushed += s->len - s->pos;
    s->pos = 0;
    s->len = 0;
    return 0;
}

int stdio64_write(int f, const void *buf, u64 len) {
    struct stdio64_file *s = stdio64_get(f);
    const unsigned char *p;
    u64 i, wrote = 0;
    if (!s || !buf) return -1;
    p = (const unsigned char *)buf;
    for (i = 0; i < len; i++) {
        if (s->len >= STDIO64_BUF) stdio64_flush(f);
        s->buf[s->len++] = p[i];
        wrote++;
        if (s->mode == STDIO64_LINE && p[i] == '\n') stdio64_flush(f);
        if (s->mode == STDIO64_NONE) stdio64_flush(f);
    }
    return (int)wrote;
}

int stdio64_read(int f, void *buf, u64 len) {
    struct stdio64_file *s = stdio64_get(f);
    unsigned char *d;
    u64 n, i;
    if (!s || !buf) return -1;
    if (s->pos >= s->len) return 0;
    n = s->len - s->pos;
    if (n > len) n = len;
    d = (unsigned char *)buf;
    for (i = 0; i < n; i++) d[i] = s->buf[s->pos + i];
    s->pos += n;
    return (int)n;
}

int stdio64_seek(int f, u64 off) {
    struct stdio64_file *s = stdio64_get(f);
    if (!s || off > s->len) return -1;
    s->pos = off;
    return 0;
}

int stdio64_close(int f) {
    struct stdio64_file *s = stdio64_get(f);
    if (!s) return -1;
    stdio64_flush(f);
    s->used = 0;
    return 0;
}
