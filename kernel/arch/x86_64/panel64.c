/* 56A: panel core — konum/boyut + eklenti sirasi + otomatik-gizleme. */
#include "arch/x86_64/longmode.h"

#define PANEL64_MAX 4
#define PANEL64_MAX_PLUGINS 16
#define PANEL64_SW 1920
#define PANEL64_SH 1080

struct panel64_plugin {
    int used;
    char name[32];
    int expand;
};

struct panel64_entry {
    int used;
    int pos;
    int size;
    int autohide;
    int opacity; /* 0..100 */
    int output;  /* monitor id, -1 = birincil */
    struct panel64_plugin plugins[PANEL64_MAX_PLUGINS];
};

static struct panel64_entry panel64_tab[PANEL64_MAX];

static void panel_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int panel_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int panel64_create(int pos, int size) {
    int i;
    if (pos < 0 || pos > 3 || size <= 0 || size > 256) return -1;
    for (i = 0; i < PANEL64_MAX; i++) {
        int j;
        if (!panel64_tab[i].used) {
            panel64_tab[i].used = 1;
            panel64_tab[i].pos = pos;
            panel64_tab[i].size = size;
            panel64_tab[i].autohide = 0;
            panel64_tab[i].opacity = 100;
            panel64_tab[i].output = -1;
            for (j = 0; j < PANEL64_MAX_PLUGINS; j++)
                panel64_tab[i].plugins[j].used = 0;
            return i;
        }
    }
    return -2;
}

static struct panel64_entry *panel64_get(int p) {
    if (p < 0 || p >= PANEL64_MAX || !panel64_tab[p].used) return 0;
    return &panel64_tab[p];
}

int panel64_add_plugin(int panel, const char *name, int expand) {
    struct panel64_entry *e = panel64_get(panel);
    int i;
    if (!e || !name) return -1;
    for (i = 0; i < PANEL64_MAX_PLUGINS; i++) {
        if (!e->plugins[i].used) {
            e->plugins[i].used = 1;
            panel_str_copy(e->plugins[i].name, name, 32);
            e->plugins[i].expand = expand ? 1 : 0;
            return 0;
        }
    }
    return -2;
}

int panel64_remove_plugin(int panel, const char *name) {
    struct panel64_entry *e = panel64_get(panel);
    int i;
    if (!e || !name) return -1;
    for (i = 0; i < PANEL64_MAX_PLUGINS; i++) {
        if (e->plugins[i].used && panel_str_eq(e->plugins[i].name, name)) {
            e->plugins[i].used = 0;
            return 0;
        }
    }
    return -2;
}

int panel64_geometry(int panel, int *x, int *y, int *w, int *h) {
    struct panel64_entry *e = panel64_get(panel);
    int gx = 0, gy = 0, gw = 0, gh = 0;
    if (!e) return -1;
    if (e->pos == PANEL64_TOP) {
        gx = 0;
        gy = 0;
        gw = PANEL64_SW;
        gh = e->size;
    } else if (e->pos == PANEL64_BOTTOM) {
        gx = 0;
        gy = PANEL64_SH - e->size;
        gw = PANEL64_SW;
        gh = e->size;
    } else if (e->pos == PANEL64_LEFT) {
        gx = 0;
        gy = 0;
        gw = e->size;
        gh = PANEL64_SH;
    } else {
        gx = PANEL64_SW - e->size;
        gy = 0;
        gw = e->size;
        gh = PANEL64_SH;
    }
    if (x) *x = gx;
    if (y) *y = gy;
    if (w) *w = gw;
    if (h) *h = gh;
    return 0;
}

int panel64_autohide(int panel, int on) {
    struct panel64_entry *e = panel64_get(panel);
    if (!e) return -1;
    e->autohide = on ? 1 : 0;
    return 0;
}

/* Eklentiyi kullanilan-sira icinde tasi (pos: 0=bas).
 * Liste sikistirilir (delikler kapanir). Donus yeni konum. */
int panel64_move_plugin(int panel, const char *name, int pos) {
    struct panel64_entry *e = panel64_get(panel);
    struct panel64_plugin tmp, order[PANEL64_MAX_PLUGINS];
    int i, n = 0, fi = -1;
    if (!e || !name) return -1;
    for (i = 0; i < PANEL64_MAX_PLUGINS; i++) {
        if (!e->plugins[i].used) continue;
        if (panel_str_eq(e->plugins[i].name, name)) fi = n;
        order[n++] = e->plugins[i];
    }
    if (fi < 0) return -2;
    if (pos < 0) pos = 0;
    if (pos >= n) pos = n - 1;
    tmp = order[fi];
    if (fi < pos) {
        for (i = fi; i < pos; i++) order[i] = order[i + 1];
        order[pos] = tmp;
    } else if (fi > pos) {
        for (i = fi; i > pos; i--) order[i] = order[i - 1];
        order[pos] = tmp;
    }
    for (i = 0; i < n; i++) e->plugins[i] = order[i];
    for (; i < PANEL64_MAX_PLUGINS; i++) e->plugins[i].used = 0;
    return pos;
}

/* Siradaki idx'inci kullanilan eklenti adi. */
int panel64_plugin_at(int panel, int idx, char *out, int max) {
    struct panel64_entry *e = panel64_get(panel);
    int i, n = 0;
    if (!e || !out || max <= 0) return -1;
    for (i = 0; i < PANEL64_MAX_PLUGINS; i++) {
        int k;
        if (!e->plugins[i].used) continue;
        if (n++ != idx) continue;
        for (k = 0; e->plugins[i].name[k] && k + 1 < max; k++)
            out[k] = e->plugins[i].name[k];
        out[k] = 0;
        return 0;
    }
    return -2;
}

int panel64_plugin_count(int panel) {
    struct panel64_entry *e = panel64_get(panel);
    int i, n = 0;
    if (!e) return -1;
    for (i = 0; i < PANEL64_MAX_PLUGINS; i++) {
        if (e->plugins[i].used) n++;
    }
    return n;
}

/* Calisma-alani disina itilen kenar (strut): panel konumuna gore. */
int panel64_struts(int panel, int *left, int *right, int *top,
                   int *bottom) {
    struct panel64_entry *e = panel64_get(panel);
    int l = 0, r = 0, t = 0, b = 0;
    if (!e) return -1;
    if (e->autohide) {
        /* Gizli panel strut birakmaz */
    } else if (e->pos == PANEL64_TOP) {
        t = e->size;
    } else if (e->pos == PANEL64_BOTTOM) {
        b = e->size;
    } else if (e->pos == PANEL64_LEFT) {
        l = e->size;
    } else {
        r = e->size;
    }
    if (left) *left = l;
    if (right) *right = r;
    if (top) *top = t;
    if (bottom) *bottom = b;
    return 0;
}

int panel64_opacity(int panel, int level) {
    struct panel64_entry *e = panel64_get(panel);
    if (!e || level < 0 || level > 100) return -1;
    e->opacity = level;
    return 0;
}

int panel64_opacity_get(int panel) {
    struct panel64_entry *e = panel64_get(panel);
    if (!e) return -1;
    return e->opacity;
}

int panel64_output(int panel, int output) {
    struct panel64_entry *e = panel64_get(panel);
    if (!e || output < -1 || output > 7) return -1;
    e->output = output;
    return 0;
}

int panel64_output_get(int panel) {
    struct panel64_entry *e = panel64_get(panel);
    if (!e) return -2;
    return e->output;
}
