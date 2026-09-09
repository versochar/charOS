/* 56I: sag-tik menusu — oge/alt-menu + konum + tetikleme. */
#include "arch/x86_64/longmode.h"

#define CTXMENU64_MAX 16
#define CTXMENU64_MAX_ITEMS 16

struct ctxmenu64_item {
    int used;
    char label[48];
    int action;
    int submenu; /* -1 = yok */
    int sensitive;
};

struct ctxmenu64_menu {
    int used;
    struct ctxmenu64_item items[CTXMENU64_MAX_ITEMS];
    int nitems;
    int x, y;
    int open;
};

static struct ctxmenu64_menu ctxmenu64_tab[CTXMENU64_MAX];

static void ctxmenu_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int ctxmenu64_create(void) {
    int i, j;
    for (i = 0; i < CTXMENU64_MAX; i++) {
        if (!ctxmenu64_tab[i].used) {
            ctxmenu64_tab[i].used = 1;
            for (j = 0; j < CTXMENU64_MAX_ITEMS; j++)
                ctxmenu64_tab[i].items[j].used = 0;
            ctxmenu64_tab[i].nitems = 0;
            ctxmenu64_tab[i].x = 0;
            ctxmenu64_tab[i].y = 0;
            ctxmenu64_tab[i].open = 0;
            return i;
        }
    }
    return -1;
}

static struct ctxmenu64_menu *ctxmenu64_get(int menu) {
    if (menu < 0 || menu >= CTXMENU64_MAX || !ctxmenu64_tab[menu].used)
        return 0;
    return &ctxmenu64_tab[menu];
}

int ctxmenu64_add_item(int menu, const char *label, int action) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    int i;
    if (!m || !label) return -1;
    for (i = 0; i < CTXMENU64_MAX_ITEMS; i++) {
        if (!m->items[i].used) {
            m->items[i].used = 1;
            ctxmenu_str_copy(m->items[i].label, label, 48);
            m->items[i].action = action;
            m->items[i].submenu = -1;
            m->items[i].sensitive = 1;
            m->nitems++;
            return i;
        }
    }
    return -2;
}

int ctxmenu64_add_submenu(int menu, const char *label, int sub) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    int idx;
    if (!m || sub < 0) return -1;
    idx = ctxmenu64_add_item(menu, label, -1);
    if (idx < 0) return idx;
    m->items[idx].submenu = sub;
    return idx;
}

int ctxmenu64_popup(int menu, int x, int y) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m) return -1;
    m->x = x;
    m->y = y;
    m->open = 1;
    return 0;
}

/* Eylem id dondurur; alt-menu ise alt id'nin negatifi (-1000-sub). */
int ctxmenu64_activate(int menu, int index) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    int n = 0, i;
    if (!m || !m->open) return -1;
    for (i = 0; i < CTXMENU64_MAX_ITEMS; i++) {
        if (!m->items[i].used || !m->items[i].sensitive) continue;
        if (n++ != index) continue;
        m->open = 0;
        if (m->items[i].submenu >= 0) return -1000 - m->items[i].submenu;
        return m->items[i].action;
    }
    return -2;
}

int ctxmenu64_remove_item(int menu, int idx) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m || idx < 0 || idx >= CTXMENU64_MAX_ITEMS) return -1;
    if (!m->items[idx].used) return -2;
    m->items[idx].used = 0;
    m->nitems--;
    return 0;
}

int ctxmenu64_set_sensitive(int menu, int idx, int on) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m || idx < 0 || idx >= CTXMENU64_MAX_ITEMS) return -1;
    if (!m->items[idx].used) return -2;
    m->items[idx].sensitive = on ? 1 : 0;
    return 0;
}

int ctxmenu64_label(int menu, int idx, char *out, int max) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    int i;
    if (!m || !out || max <= 0) return -1;
    if (idx < 0 || idx >= CTXMENU64_MAX_ITEMS) return -2;
    if (!m->items[idx].used) return -3;
    ctxmenu_str_copy(out, m->items[idx].label, max);
    return 0;
}

int ctxmenu64_action(int menu, int idx, int *out) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m || !out) return -1;
    if (idx < 0 || idx >= CTXMENU64_MAX_ITEMS) return -2;
    if (!m->items[idx].used) return -3;
    *out = m->items[idx].action;
    return 0;
}

int ctxmenu64_count(int menu) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m) return -1;
    return m->nitems;
}

int ctxmenu64_close(int menu) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m) return -1;
    m->open = 0;
    return 0;
}

int ctxmenu64_pos(int menu, int *x, int *y) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    if (!m) return -1;
    if (x) *x = m->x;
    if (y) *y = m->y;
    return 0;
}

int ctxmenu64_clear(int menu) {
    struct ctxmenu64_menu *m = ctxmenu64_get(menu);
    int i;
    if (!m) return -1;
    for (i = 0; i < CTXMENU64_MAX_ITEMS; i++) {
        m->items[i].used = 0;
    }
    m->nitems = 0;
    m->open = 0;
    return 0;
}
