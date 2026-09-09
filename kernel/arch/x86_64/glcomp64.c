/* 47F: 3D compositor GL — yuzeyler + kirli bolge + sunum karmasi. */
#include "arch/x86_64/longmode.h"

#define GLCOMP64_MAX_SURF 16

struct glcomp64_surf {
    int used;
    int z;
    u32 w, h;
    u32 color;
    u32 dx0, dy0, dx1, dy1;
    int dirty;
};

static struct glcomp64_surf glcomp64_tab[GLCOMP64_MAX_SURF];
static u64 glcomp64_vsync_n = 0;

int glcomp64_surface(u32 w, u32 h, u32 color) {
    int i;
    if (!w || !h || w > 4096 || h > 4096) return -1;
    for (i = 0; i < GLCOMP64_MAX_SURF; i++) {
        if (!glcomp64_tab[i].used) {
            glcomp64_tab[i].used = 1;
            glcomp64_tab[i].z = i;
            glcomp64_tab[i].w = w;
            glcomp64_tab[i].h = h;
            glcomp64_tab[i].color = color;
            glcomp64_tab[i].dx0 = 0;
            glcomp64_tab[i].dy0 = 0;
            glcomp64_tab[i].dx1 = w;
            glcomp64_tab[i].dy1 = h;
            glcomp64_tab[i].dirty = 1;
            return i;
        }
    }
    return -2;
}

int glcomp64_damage(int id, u32 x, u32 y, u32 w, u32 h) {
    struct glcomp64_surf *s;
    if (id < 0 || id >= GLCOMP64_MAX_SURF || !glcomp64_tab[id].used)
        return -1;
    s = &glcomp64_tab[id];
    if (x >= s->w || y >= s->h) return -2;
    if (x + w > s->w) w = s->w - x;
    if (y + h > s->h) h = s->h - y;
    if (!s->dirty) {
        s->dx0 = x;
        s->dy0 = y;
        s->dx1 = x + w;
        s->dy1 = y + h;
    } else {
        if (x < s->dx0) s->dx0 = x;
        if (y < s->dy0) s->dy0 = y;
        if (x + w > s->dx1) s->dx1 = x + w;
        if (y + h > s->dy1) s->dy1 = y + h;
    }
    s->dirty = 1;
    return 0;
}

/* Sunum: kirli yuzey renklerinin z-sirali FNV karmasi (test gozlemi). */
u32 glcomp64_present(void) {
    u32 h = 2166136261UL;
    int z, i;
    for (z = 0; z < GLCOMP64_MAX_SURF; z++) {
        for (i = 0; i < GLCOMP64_MAX_SURF; i++) {
            struct glcomp64_surf *s = &glcomp64_tab[i];
            u32 v;
            if (!s->used || s->z != z || !s->dirty) continue;
            v = s->color ^ (s->dx1 - s->dx0) ^ (s->dy1 - s->dy0);
            h ^= v & 0xFF;
            h *= 16777619UL;
            h ^= (v >> 8) & 0xFF;
            h *= 16777619UL;
            h ^= (v >> 16) & 0xFF;
            h *= 16777619UL;
            h ^= (v >> 24) & 0xFF;
            h *= 16777619UL;
            s->dirty = 0;
        }
    }
    glcomp64_vsync_n++;
    return h;
}

u64 glcomp64_vsync(void) { return glcomp64_vsync_n; }
