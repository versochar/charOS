/* 55B: X11 compat — pencere esleme + ozellik deposu + haritalama. */
#include "arch/x86_64/longmode.h"

#define XCOMPAT64_MAX_WIN 32
#define XCOMPAT64_MAX_PROP 64

struct xcompat64_win {
    int used;
    u32 xid;
    int surf;
    int mapped;
};

struct xcompat64_prop {
    int used;
    u32 xid;
    char name[32];
    char val[96];
};

static struct xcompat64_win xcompat64_wins[XCOMPAT64_MAX_WIN];
static struct xcompat64_prop xcompat64_props[XCOMPAT64_MAX_PROP];

static void xc_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int xc_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct xcompat64_win *xcompat64_find(u32 xid) {
    int i;
    for (i = 0; i < XCOMPAT64_MAX_WIN; i++) {
        if (xcompat64_wins[i].used && xcompat64_wins[i].xid == xid)
            return &xcompat64_wins[i];
    }
    return 0;
}

int xcompat64_window_create(u32 xid, int surf) {
    struct xcompat64_win *w = xcompat64_find(xid);
    int i;
    if (!xid) return -1;
    if (w) {
        w->surf = surf;
        return 0;
    }
    for (i = 0; i < XCOMPAT64_MAX_WIN; i++) {
        if (!xcompat64_wins[i].used) {
            xcompat64_wins[i].used = 1;
            xcompat64_wins[i].xid = xid;
            xcompat64_wins[i].surf = surf;
            xcompat64_wins[i].mapped = 0;
            return 0;
        }
    }
    return -2;
}

int xcompat64_prop_set(u32 xid, const char *name, const char *val) {
    int i;
    if (!xid || !name) return -1;
    for (i = 0; i < XCOMPAT64_MAX_PROP; i++) {
        if (xcompat64_props[i].used && xcompat64_props[i].xid == xid &&
            xc_str_eq(xcompat64_props[i].name, name)) {
            xc_str_copy(xcompat64_props[i].val, val ? val : "", 96);
            return 0;
        }
    }
    for (i = 0; i < XCOMPAT64_MAX_PROP; i++) {
        if (!xcompat64_props[i].used) {
            xcompat64_props[i].used = 1;
            xcompat64_props[i].xid = xid;
            xc_str_copy(xcompat64_props[i].name, name, 32);
            xc_str_copy(xcompat64_props[i].val, val ? val : "", 96);
            return 0;
        }
    }
    return -2;
}

int xcompat64_prop_get(u32 xid, const char *name, char *out, int max) {
    int i;
    if (!name || !out || max <= 0) return -1;
    for (i = 0; i < XCOMPAT64_MAX_PROP; i++) {
        if (xcompat64_props[i].used && xcompat64_props[i].xid == xid &&
            xc_str_eq(xcompat64_props[i].name, name)) {
            xc_str_copy(out, xcompat64_props[i].val, max);
            return 0;
        }
    }
    return -2;
}

int xcompat64_map(u32 xid) {
    struct xcompat64_win *w = xcompat64_find(xid);
    if (!w) return -1;
    w->mapped = 1;
    return 0;
}

int xcompat64_unmap(u32 xid) {
    struct xcompat64_win *w = xcompat64_find(xid);
    if (!w) return -1;
    w->mapped = 0;
    return 0;
}

int xcompat64_mapped(u32 xid) {
    struct xcompat64_win *w = xcompat64_find(xid);
    if (!w) return -1;
    return w->mapped;
}
