/* 38G: SELinux context — yol etiketi + tur gecisi (37F kurallariyla). */
#include "arch/x86_64/longmode.h"

#define SELCTX64_MAX_LABEL 32
#define SELCTX64_MAX_TRANS 32

struct selctx64_label {
    int used;
    char prefix[64];
    char ctx[128];
};

struct selctx64_trans {
    int used;
    char src[32];
    char obj[32];
    char cls[32];
    char dst[32];
};

static struct selctx64_label selctx64_labels[SELCTX64_MAX_LABEL];
static struct selctx64_trans selctx64_trans[SELCTX64_MAX_TRANS];

static void sctx_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int sctx_prefix(const char *prefix, const char *path) {
    int i;
    for (i = 0;; i++) {
        if (!prefix[i]) return 1;
        if (prefix[i] != path[i]) return 0;
        if (!path[i]) return 0;
    }
}

int selinuxctx64_add(const char *prefix, const char *ctx) {
    int i;
    if (!prefix || !ctx) return -1;
    for (i = 0; i < SELCTX64_MAX_LABEL; i++) {
        if (!selctx64_labels[i].used) {
            selctx64_labels[i].used = 1;
            sctx_copy(selctx64_labels[i].prefix, prefix, 64);
            sctx_copy(selctx64_labels[i].ctx, ctx, 128);
            return 0;
        }
    }
    return -2;
}

/* En uzun one Ekli etiket kazanir. */
int selinuxctx64_label(const char *path, char *out, int max) {
    int i, best = -1, bestlen = -1, len;
    if (!path || !out || max <= 0) return -1;
    for (i = 0; i < SELCTX64_MAX_LABEL; i++) {
        if (!selctx64_labels[i].used) continue;
        if (!sctx_prefix(selctx64_labels[i].prefix, path)) continue;
        for (len = 0; selctx64_labels[i].prefix[len]; len++) {
        }
        if (len > bestlen) {
            bestlen = len;
            best = i;
        }
    }
    if (best < 0) return -2;
    sctx_copy(out, selctx64_labels[best].ctx, max);
    return 0;
}

int selinuxctx64_add_trans(const char *src, const char *obj,
                           const char *cls, const char *dst) {
    int i;
    if (!src || !obj || !cls || !dst) return -1;
    for (i = 0; i < SELCTX64_MAX_TRANS; i++) {
        if (!selctx64_trans[i].used) {
            selctx64_trans[i].used = 1;
            sctx_copy(selctx64_trans[i].src, src, 32);
            sctx_copy(selctx64_trans[i].obj, obj, 32);
            sctx_copy(selctx64_trans[i].cls, cls, 32);
            sctx_copy(selctx64_trans[i].dst, dst, 32);
            return 0;
        }
    }
    return -2;
}

static int sctx_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int selinuxctx64_trans(const char *src, const char *obj, const char *cls,
                       char *out, int max) {
    int i;
    if (!src || !obj || !cls || !out || max <= 0) return -1;
    for (i = 0; i < SELCTX64_MAX_TRANS; i++) {
        struct selctx64_trans *t = &selctx64_trans[i];
        if (!t->used) continue;
        if (!sctx_eq(t->src, src)) continue;
        if (!sctx_eq(t->obj, obj)) continue;
        if (!sctx_eq(t->cls, cls)) continue;
        sctx_copy(out, t->dst, max);
        return 0;
    }
    return -2; /* gecis yok */
}
