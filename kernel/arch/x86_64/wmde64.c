/* 56B: pencere yoneticisi — odak yigini + durum + calisma-alani. */
#include "arch/x86_64/longmode.h"

#define WMDE64_MAX 32

struct wmde64_win {
    int used;
    int id;
    char title[64];
    int x, y, w, h;
    int minimized;
    int maximized;
    int fullscreen;
    int sticky;
    int ws;
    int z; /* kucuk = altta */
    int minw, minh, maxw, maxh; /* 0 = sinirsiz */
    /* Büyütme öncesi geometri */
    int sx, sy, sw, sh;
};

static struct wmde64_win wmde64_tab[WMDE64_MAX];
static int wmde64_next_id = 1;
static int wmde64_focus_id = -1;
static int wmde64_ws = 0;
static int wmde64_z_top = 0;

static void wmde_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct wmde64_win *wmde_find(int id) {
    int i;
    for (i = 0; i < WMDE64_MAX; i++) {
        if (wmde64_tab[i].used && wmde64_tab[i].id == id)
            return &wmde64_tab[i];
    }
    return 0;
}

int wmde64_open(const char *title, int x, int y, int w, int h) {
    int i;
    if (!title || w <= 0 || h <= 0) return -1;
    for (i = 0; i < WMDE64_MAX; i++) {
        if (!wmde64_tab[i].used) {
            wmde64_tab[i].used = 1;
            wmde64_tab[i].id = wmde64_next_id++;
            wmde_str_copy(wmde64_tab[i].title, title, 64);
            wmde64_tab[i].x = x;
            wmde64_tab[i].y = y;
            wmde64_tab[i].w = w;
            wmde64_tab[i].h = h;
            wmde64_tab[i].minimized = 0;
            wmde64_tab[i].maximized = 0;
            wmde64_tab[i].fullscreen = 0;
            wmde64_tab[i].sticky = 0;
            wmde64_tab[i].ws = wmde64_ws;
            wmde64_tab[i].z = wmde64_z_top++;
            wmde64_tab[i].minw = 0;
            wmde64_tab[i].minh = 0;
            wmde64_tab[i].maxw = 0;
            wmde64_tab[i].maxh = 0;
            wmde64_focus_id = wmde64_tab[i].id;
            return wmde64_tab[i].id;
        }
    }
    return -2;
}

int wmde64_close(int id) {
    struct wmde64_win *w = wmde_find(id);
    int i, best = -1;
    if (!w) return -1;
    w->used = 0;
    if (wmde64_focus_id == id) {
        wmde64_focus_id = -1;
        for (i = 0; i < WMDE64_MAX; i++) {
            if (wmde64_tab[i].used && wmde64_tab[i].ws == wmde64_ws &&
                !wmde64_tab[i].minimized)
                best = wmde64_tab[i].id;
        }
        wmde64_focus_id = best;
    }
    return 0;
}

int wmde64_focus(int id) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    if (w->minimized) w->minimized = 0;
    w->z = wmde64_z_top++;
    wmde64_focus_id = id;
    return 0;
}

int wmde64_focused(void) { return wmde64_focus_id; }

int wmde64_minimize(int id, int on) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    w->minimized = on ? 1 : 0;
    if (on && wmde64_focus_id == id) wmde64_focus_id = -1;
    return 0;
}

int wmde64_maximize(int id, int on) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    if (on && !w->maximized) {
        w->sx = w->x;
        w->sy = w->y;
        w->sw = w->w;
        w->sh = w->h;
        w->x = 0;
        w->y = 0;
        w->w = 1920;
        w->h = 1080;
        w->maximized = 1;
    } else if (!on && w->maximized) {
        w->x = w->sx;
        w->y = w->sy;
        w->w = w->sw;
        w->h = w->sh;
        w->maximized = 0;
    }
    return 0;
}

int wmde64_move(int id, int x, int y) {
    struct wmde64_win *w = wmde_find(id);
    if (!w || w->maximized || w->fullscreen) return -1;
    w->x = x;
    w->y = y;
    return 0;
}

static void wmde64_clamp(struct wmde64_win *win) {
    if (win->minw > 0 && win->w < win->minw) win->w = win->minw;
    if (win->minh > 0 && win->h < win->minh) win->h = win->minh;
    if (win->maxw > 0 && win->w > win->maxw) win->w = win->maxw;
    if (win->maxh > 0 && win->h > win->maxh) win->h = win->maxh;
}

int wmde64_resize(int id, int w, int h) {
    struct wmde64_win *win = wmde_find(id);
    if (!win || win->maximized || win->fullscreen || w <= 0 || h <= 0)
        return -1;
    win->w = w;
    win->h = h;
    wmde64_clamp(win);
    return 0;
}

int wmde64_workspace(int ws) {
    if (ws < 0 || ws > 9) return -1;
    wmde64_ws = ws;
    wmde64_focus_id = -1;
    return 0;
}

int wmde64_current_workspace(void) { return wmde64_ws; }

/* One cikar (en uste); donus yeni z. */
int wmde64_raise(int id) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    w->z = wmde64_z_top++;
    return w->z;
}

/* Alta indir (bu alandaki en dusuk z); donus yeni z. */
int wmde64_lower(int id) {
    struct wmde64_win *w = wmde_find(id);
    int i, minz;
    if (!w) return -1;
    minz = w->z;
    for (i = 0; i < WMDE64_MAX; i++) {
        if (!wmde64_tab[i].used) continue;
        if (wmde64_tab[i].ws != w->ws && !wmde64_tab[i].sticky) continue;
        if (wmde64_tab[i].z < minz) minz = wmde64_tab[i].z;
    }
    w->z = minz - 1;
    return w->z;
}

/* Alttan-uste z sirasi (gorunur: ayni alan veya yapiskan, kucultulmemis).
 * Donus adet. */
int wmde64_stack(int ws, int *out, int max) {
    int ids[WMDE64_MAX], n = 0, i, j;
    if (!out || max <= 0) return -1;
    for (i = 0; i < WMDE64_MAX; i++) {
        if (!wmde64_tab[i].used) continue;
        if (wmde64_tab[i].minimized) continue;
        if (wmde64_tab[i].ws != ws && !wmde64_tab[i].sticky) continue;
        ids[n++] = i;
    }
    /* Kabarcik: z artan */
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            if (wmde64_tab[ids[j]].z < wmde64_tab[ids[i]].z) {
                int t = ids[i];
                ids[i] = ids[j];
                ids[j] = t;
            }
        }
    }
    if (n > max) n = max;
    for (i = 0; i < n; i++) out[i] = wmde64_tab[ids[i]].id;
    return n;
}

/* Yapiskansa tum alanlarda gorunur. */
int wmde64_sticky(int id, int on) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    w->sticky = on ? 1 : 0;
    return 0;
}

/* Tam-ekran: panel dahil kaplar, kucultme/buyutmeden bagimsiz bayrak. */
int wmde64_fullscreen(int id, int on) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    if (on && !w->fullscreen) {
        if (!w->maximized) {
            w->sx = w->x;
            w->sy = w->y;
            w->sw = w->w;
            w->sh = w->h;
        }
        w->x = 0;
        w->y = 0;
        w->w = 1920;
        w->h = 1080;
        w->fullscreen = 1;
    } else if (!on && w->fullscreen) {
        w->x = w->sx;
        w->y = w->sy;
        w->w = w->sw;
        w->h = w->sh;
        w->fullscreen = 0;
    }
    return 0;
}

/* Boyut kisitlari (0 = sinirsiz); mevcut geometri kirpilir. */
int wmde64_constrain(int id, int minw, int minh, int maxw, int maxh) {
    struct wmde64_win *w = wmde_find(id);
    if (!w) return -1;
    if ((minw && maxw && minw > maxw) || (minh && maxh && minh > maxh))
        return -2;
    w->minw = minw;
    w->minh = minh;
    w->maxw = maxw;
    w->maxh = maxh;
    wmde64_clamp(w);
    return 0;
}

int wmde64_geom(int id, int *x, int *y, int *w, int *h) {
    struct wmde64_win *win = wmde_find(id);
    if (!win) return -1;
    if (x) *x = win->x;
    if (y) *y = win->y;
    if (w) *w = win->w;
    if (h) *h = win->h;
    return 0;
}

int wmde64_count(void) {
    int i, n = 0;
    for (i = 0; i < WMDE64_MAX; i++) {
        if (wmde64_tab[i].used) n++;
    }
    return n;
}
