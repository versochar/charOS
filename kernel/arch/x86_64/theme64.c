/* 56D: tema motoru — ozellik deposu + etkin tema + renk cozumu. */
#include "arch/x86_64/longmode.h"

#define THEME64_MAX 8
#define THEME64_MAX_PROPS 32

struct theme64_prop {
    int used;
    char key[48];
    char val[64];
};

struct theme64_entry {
    int used;
    char name[32];
    char parent[32]; /* miras zinciri (bos = yok) */
    int dark;        /* 1=koyu varyant */
    struct theme64_prop props[THEME64_MAX_PROPS];
};

static struct theme64_entry theme64_tab[THEME64_MAX];
static char theme64_cur[32];

static void theme_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int theme_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct theme64_entry *theme64_find(const char *name) {
    int i;
    for (i = 0; i < THEME64_MAX; i++) {
        if (theme64_tab[i].used && theme_str_eq(theme64_tab[i].name, name))
            return &theme64_tab[i];
    }
    return 0;
}

int theme64_add(const char *name) {
    int i;
    if (!name) return -1;
    if (theme64_find(name)) return 0;
    for (i = 0; i < THEME64_MAX; i++) {
        int j;
        if (!theme64_tab[i].used) {
            theme64_tab[i].used = 1;
            theme_str_copy(theme64_tab[i].name, name, 32);
            theme64_tab[i].parent[0] = 0;
            theme64_tab[i].dark = 0;
            for (j = 0; j < THEME64_MAX_PROPS; j++)
                theme64_tab[i].props[j].used = 0;
            return 0;
        }
    }
    return -2;
}

int theme64_set_prop(const char *theme, const char *key, const char *val) {
    struct theme64_entry *t = theme64_find(theme);
    int i;
    if (!t || !key) return -1;
    for (i = 0; i < THEME64_MAX_PROPS; i++) {
        if (t->props[i].used && theme_str_eq(t->props[i].key, key)) {
            theme_str_copy(t->props[i].val, val ? val : "", 64);
            return 0;
        }
    }
    for (i = 0; i < THEME64_MAX_PROPS; i++) {
        if (!t->props[i].used) {
            t->props[i].used = 1;
            theme_str_copy(t->props[i].key, key, 48);
            theme_str_copy(t->props[i].val, val ? val : "", 64);
            return 0;
        }
    }
    return -2;
}

/* Miras zinciriyle ozellik cozumu (dongu korumali). Donus 1=miras. */
int theme64_get_prop(const char *theme, const char *key, char *out,
                     int max) {
    struct theme64_entry *t = theme64_find(theme);
    int i, depth = 0;
    if (!t || !key || !out || max <= 0) return -1;
    while (t && depth < THEME64_MAX) {
        for (i = 0; i < THEME64_MAX_PROPS; i++) {
            if (t->props[i].used && theme_str_eq(t->props[i].key, key)) {
                theme_str_copy(out, t->props[i].val, max);
                return depth ? 1 : 0;
            }
        }
        if (!t->parent[0]) break;
        t = theme64_find(t->parent);
        depth++;
    }
    return -2;
}

int theme64_inherit(const char *child, const char *parent) {
    struct theme64_entry *c = theme64_find(child);
    struct theme64_entry *p;
    const char *walk;
    int depth = 0;
    if (!c || !parent) return -1;
    p = theme64_find(parent);
    if (!p) return -2;
    /* Dongu denetimi: parent zinciri child'a donmemeli */
    walk = parent;
    while (walk && depth < THEME64_MAX) {
        struct theme64_entry *w;
        if (theme_str_eq(walk, child)) return -3;
        w = theme64_find(walk);
        if (!w || !w->parent[0]) break;
        walk = w->parent;
        depth++;
    }
    theme_str_copy(c->parent, parent, 32);
    return 0;
}

int theme64_variant(const char *theme, int dark) {
    struct theme64_entry *t = theme64_find(theme);
    if (!t) return -1;
    t->dark = dark ? 1 : 0;
    return 0;
}

int theme64_is_dark(const char *theme) {
    struct theme64_entry *t = theme64_find(theme);
    if (!t) return -1;
    return t->dark;
}

static u32 theme64_chan(u32 c, int amt, int up) {
    int v = (int)c + (up ? amt : -amt);
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return (u32)v;
}

u32 theme64_lighten(u32 color, int amt) {
    u32 r, g, b;
    if (amt < 0) amt = 0;
    if (amt > 255) amt = 255;
    r = theme64_chan((color >> 16) & 0xFF, amt, 1);
    g = theme64_chan((color >> 8) & 0xFF, amt, 1);
    b = theme64_chan(color & 0xFF, amt, 1);
    return (r << 16) | (g << 8) | b;
}

u32 theme64_darken(u32 color, int amt) {
    u32 r, g, b;
    if (amt < 0) amt = 0;
    if (amt > 255) amt = 255;
    r = theme64_chan((color >> 16) & 0xFF, amt, 0);
    g = theme64_chan((color >> 8) & 0xFF, amt, 0);
    b = theme64_chan(color & 0xFF, amt, 0);
    return (r << 16) | (g << 8) | b;
}

u32 theme64_blend(u32 a, u32 b, int t) {
    u32 ar, ag, ab, br, bg, bb, r, g, bl;
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    ar = (a >> 16) & 0xFF;
    ag = (a >> 8) & 0xFF;
    ab = a & 0xFF;
    br = (b >> 16) & 0xFF;
    bg = (b >> 8) & 0xFF;
    bb = b & 0xFF;
    r = (ar * (u32)(256 - t) + br * (u32)t) / 256;
    g = (ag * (u32)(256 - t) + bg * (u32)t) / 256;
    bl = (ab * (u32)(256 - t) + bb * (u32)t) / 256;
    return (r << 16) | (g << 8) | bl;
}

/* WCAG bagil parlaklik (x1000 olcekli tamsayi). */
static u64 theme64_luminance(u32 color) {
    u64 r = (color >> 16) & 0xFF;
    u64 g = (color >> 8) & 0xFF;
    u64 b = color & 0xFF;
    /* sRGB dogrusallastirma yaklasigi: (c/255)^2.2, olcekli tamsayi */
    u64 lr = (r * r * 1000) / (255 * 255);
    u64 lg = (g * g * 1000) / (255 * 255);
    u64 lb = (b * b * 1000) / (255 * 255);
    return (2126 * lr + 7152 * lg + 722 * lb) / 10000;
}

/* Kontrast orani x100 (beyaz/siyah = 2100). */
u64 theme64_contrast(u32 fg, u32 bg) {
    u64 l1 = theme64_luminance(fg);
    u64 l2 = theme64_luminance(bg);
    u64 hi = l1 > l2 ? l1 : l2;
    u64 lo = l1 > l2 ? l2 : l1;
    return ((hi + 50) * 100) / (lo + 50);
}

int theme64_list(char out[][32], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < THEME64_MAX && n < max; i++) {
        int k;
        if (!theme64_tab[i].used) continue;
        for (k = 0; theme64_tab[i].name[k] && k < 31; k++)
            out[n][k] = theme64_tab[i].name[k];
        out[n][k] = 0;
        n++;
    }
    return n;
}

int theme64_remove_prop(const char *theme, const char *key) {
    struct theme64_entry *t = theme64_find(theme);
    int i;
    if (!t || !key) return -1;
    for (i = 0; i < THEME64_MAX_PROPS; i++) {
        if (t->props[i].used && theme_str_eq(t->props[i].key, key)) {
            t->props[i].used = 0;
            return 0;
        }
    }
    return -2;
}

int theme64_copy(const char *src, const char *dst) {
    struct theme64_entry *s = theme64_find(src);
    int i;
    if (!s || !dst) return -1;
    if (theme64_add(dst) != 0) return -1;
    {
        struct theme64_entry *d = theme64_find(dst);
        if (!d) return -1;
        theme_str_copy(d->parent, s->parent, 32);
        d->dark = s->dark;
        for (i = 0; i < THEME64_MAX_PROPS; i++) {
            int k;
            d->props[i].used = s->props[i].used;
            for (k = 0; k < 48; k++)
                d->props[i].key[k] = s->props[i].key[k];
            for (k = 0; k < 64; k++)
                d->props[i].val[k] = s->props[i].val[k];
        }
    }
    return 0;
}

int theme64_apply(const char *name) {
    if (!theme64_find(name)) return -1;
    theme_str_copy(theme64_cur, name, 32);
    return 0;
}

int theme64_active(char *out, int max) {
    if (!theme64_cur[0]) return -1;
    if (out && max > 0) theme_str_copy(out, theme64_cur, max);
    return 0;
}

static int theme_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* "#rrggbb" -> 0x00RRGGBB; hataliysa 0. */
u32 theme64_color(const char *hex) {
    u32 v = 0;
    int i;
    if (!hex || hex[0] != '#') return 0;
    for (i = 1; i <= 6; i++) {
        int d = theme_hex(hex[i]);
        if (d < 0) return 0;
        v = (v << 4) | (u32)d;
    }
    if (hex[7]) return 0;
    return v;
}
