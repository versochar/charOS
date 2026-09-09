/* 37F: SELinux skeleton — baglam ayrıştırma + kural tablosu + kip. */
#include "arch/x86_64/longmode.h"

#define SELINUX64_MAX_RULES 64

struct selinux64_rule {
    int used;
    char src[32];
    char dst[32];
    char cls[32];
    u32 perms;
};

static struct selinux64_rule selinux64_tab[SELINUX64_MAX_RULES];
static int selinux64_enforce = 1;

static void str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int selinux64_parse(const char *ctx, struct selinux64_ctx *out) {
    const char *p;
    int f, i;
    char *dsts[4];
    if (!ctx || !out) return -1;
    dsts[0] = out->user;
    dsts[1] = out->role;
    dsts[2] = out->type;
    dsts[3] = out->level;
    p = ctx;
    for (f = 0; f < 4; f++) {
        i = 0;
        while (*p && *p != ':' && i < 31) dsts[f][i++] = *p++;
        dsts[f][i] = 0;
        if (f < 3) {
            if (*p != ':') return -2; /* eksik alan */
            p++;
        }
    }
    if (!out->user[0] || !out->role[0] || !out->type[0]) return -3;
    return 0;
}

int selinux64_add_rule(const char *src, const char *dst, const char *cls,
                       u32 perms) {
    int i;
    if (!src || !dst || !cls) return -1;
    for (i = 0; i < SELINUX64_MAX_RULES; i++) {
        if (!selinux64_tab[i].used) {
            str_copy(selinux64_tab[i].src, src, 32);
            str_copy(selinux64_tab[i].dst, dst, 32);
            str_copy(selinux64_tab[i].cls, cls, 32);
            selinux64_tab[i].perms = perms;
            selinux64_tab[i].used = 1;
            return 0;
        }
    }
    return -2;
}

int selinux64_check(const char *src, const char *dst, const char *cls,
                    u32 perm) {
    int i;
    if (!src || !dst || !cls) return -1;
    for (i = 0; i < SELINUX64_MAX_RULES; i++) {
        if (!selinux64_tab[i].used) continue;
        if (!str_eq(selinux64_tab[i].src, src)) continue;
        if (!str_eq(selinux64_tab[i].dst, dst)) continue;
        if (!str_eq(selinux64_tab[i].cls, cls)) continue;
        if (selinux64_tab[i].perms & perm) return 1;
        return 0;
    }
    return 0; /* kural yoksa engelli */
}

void selinux64_enforcing(int on) { selinux64_enforce = on ? 1 : 0; }
int selinux64_is_enforcing(void) { return selinux64_enforce; }
