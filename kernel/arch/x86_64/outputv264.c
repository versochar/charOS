/* 55D: output v2 — ciktilar + kip + yerlesim + acma/kapama. */
#include "arch/x86_64/longmode.h"

#define OUTPUTV264_MAX 8

struct outputv264_entry {
    int used;
    char name[16];
    u32 w, h, refresh;
    int x, y, scale;
    int enabled;
};

static struct outputv264_entry outputv264_tab[OUTPUTV264_MAX];

static void output_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int outputv264_add(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < OUTPUTV264_MAX; i++) {
        if (!outputv264_tab[i].used) {
            outputv264_tab[i].used = 1;
            output_str_copy(outputv264_tab[i].name, name, 16);
            outputv264_tab[i].w = 0;
            outputv264_tab[i].h = 0;
            outputv264_tab[i].refresh = 0;
            outputv264_tab[i].x = 0;
            outputv264_tab[i].y = 0;
            outputv264_tab[i].scale = 1;
            outputv264_tab[i].enabled = 0;
            return i;
        }
    }
    return -2;
}

static struct outputv264_entry *outputv264_get(int id) {
    if (id < 0 || id >= OUTPUTV264_MAX || !outputv264_tab[id].used)
        return 0;
    return &outputv264_tab[id];
}

int outputv264_mode(int id, u32 w, u32 h, u32 refresh) {
    struct outputv264_entry *o = outputv264_get(id);
    if (!o || !w || !h) return -1;
    o->w = w;
    o->h = h;
    o->refresh = refresh;
    return 0;
}

int outputv264_layout(int id, int x, int y, int scale) {
    struct outputv264_entry *o = outputv264_get(id);
    if (!o || scale < 1 || scale > 4) return -1;
    o->x = x;
    o->y = y;
    o->scale = scale;
    return 0;
}

int outputv264_enable(int id, int on) {
    struct outputv264_entry *o = outputv264_get(id);
    if (!o) return -1;
    if (on && (!o->w || !o->h)) return -2; /* kipsiz acilamaz */
    o->enabled = on ? 1 : 0;
    return 0;
}

int outputv264_list(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < OUTPUTV264_MAX && n < max; i++) {
        if (outputv264_tab[i].used && outputv264_tab[i].enabled)
            ids[n++] = i;
    }
    return n;
}
