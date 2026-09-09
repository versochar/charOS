/* 55H: security context — uygulama izinleri + portal istegi. */
#include "arch/x86_64/longmode.h"

#define SECCTX64_MAX_APP 16

struct secctx64_app {
    int used;
    char appid[48];
    int sandbox;
    int perms;
    int pending;
};

static struct secctx64_app secctx64_tab[SECCTX64_MAX_APP];

static void secctx_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int secctx_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct secctx64_app *secctx64_find(const char *appid) {
    int i;
    for (i = 0; i < SECCTX64_MAX_APP; i++) {
        if (secctx64_tab[i].used &&
            secctx_str_eq(secctx64_tab[i].appid, appid))
            return &secctx64_tab[i];
    }
    return 0;
}

int secctx64_register(const char *appid, int sandbox) {
    struct secctx64_app *a = secctx64_find(appid);
    int i;
    if (!appid) return -1;
    if (a) {
        a->sandbox = sandbox;
        return 0;
    }
    for (i = 0; i < SECCTX64_MAX_APP; i++) {
        if (!secctx64_tab[i].used) {
            secctx64_tab[i].used = 1;
            secctx_str_copy(secctx64_tab[i].appid, appid, 48);
            secctx64_tab[i].sandbox = sandbox;
            secctx64_tab[i].perms = 0;
            secctx64_tab[i].pending = 0;
            return 0;
        }
    }
    return -2;
}

int secctx64_grant(const char *appid, int perm) {
    struct secctx64_app *a = secctx64_find(appid);
    if (!a || perm <= 0) return -1;
    a->perms |= perm;
    a->pending &= ~perm;
    return 0;
}

int secctx64_revoke(const char *appid, int perm) {
    struct secctx64_app *a = secctx64_find(appid);
    if (!a || perm <= 0) return -1;
    a->perms &= ~perm;
    return 0;
}

int secctx64_check(const char *appid, int perm) {
    struct secctx64_app *a = secctx64_find(appid);
    if (!a || perm <= 0) return 0;
    return (a->perms & perm) == perm ? 1 : 0;
}

/* Portal istegi: henuz verilmeyen izni beklemeye al. 1=bekliyor. */
int secctx64_prompt(const char *appid, int perm) {
    struct secctx64_app *a = secctx64_find(appid);
    if (!a || perm <= 0) return -1;
    if ((a->perms & perm) == perm) return 0; /* zaten var */
    a->pending |= perm;
    return 1;
}
