/* 47E: VRAM 512MB optimizasyon — hizali kesitler + parcalanma + tasfiye. */
#include "arch/x86_64/longmode.h"

#define VRAM64_MAX_SEG 256

struct vram64_seg {
    int used;
    int free; /* 1=bos */
    int purgeable;
    u64 addr;
    u64 size;
};

static struct vram64_seg vram64_tab[VRAM64_MAX_SEG];
static u64 vram64_total = 0;
static u64 vram64_cursor = 0;
static int vram64_ready = 0;

static u64 vram64_align_up(u64 v, u64 a) {
    if (!a) return v;
    return (v + a - 1) & ~(a - 1);
}

/* Bos kesit ekle (birlestirme yok — skeleton). */
static void vram64_add_free(u64 addr, u64 size) {
    int j;
    if (!size) return;
    for (j = 0; j < VRAM64_MAX_SEG; j++) {
        if (!vram64_tab[j].used) {
            vram64_tab[j].used = 1;
            vram64_tab[j].free = 1;
            vram64_tab[j].purgeable = 0;
            vram64_tab[j].addr = addr;
            vram64_tab[j].size = size;
            return;
        }
    }
}

int vram64_init(u64 size) {
    int i;
    if (!size || size > 2ULL * 1024 * 1024 * 1024) return -1;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        vram64_tab[i].used = 0;
        vram64_tab[i].free = 0;
        vram64_tab[i].purgeable = 0;
        vram64_tab[i].addr = 0;
        vram64_tab[i].size = 0;
    }
    /* Adres 0 gecersiz sentinel'dir: ilk 4K tarama-payina ayrilir. */
    vram64_tab[0].used = 1;
    vram64_tab[0].free = 0;
    vram64_tab[0].purgeable = 0;
    vram64_tab[0].addr = 0;
    vram64_tab[0].size = size > 4096 ? 4096 : size;
    if (size > 4096)
        vram64_add_free(4096, size - 4096);
    vram64_total = size;
    vram64_cursor = 0;
    vram64_ready = 1;
    return 0;
}

u64 vram64_alloc(u64 size, u64 align) {
    /* En-iyi-uyum (best-fit): en kucuk yeterli bos kesit. */
    int i, best = -1;
    u64 best_waste = 0;
    u64 a;
    if (!vram64_ready || !size) return 0;
    if (!align) align = 4096;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        u64 cur_a, waste;
        if (!vram64_tab[i].used || !vram64_tab[i].free) continue;
        cur_a = vram64_align_up(vram64_tab[i].addr, align);
        if (cur_a - vram64_tab[i].addr + size > vram64_tab[i].size)
            continue;
        waste = vram64_tab[i].size - (cur_a - vram64_tab[i].addr) - size;
        if (best < 0 || waste < best_waste) {
            best = i;
            best_waste = waste;
        }
    }
    if (best < 0) return 0;
    {
        u64 seg_addr = vram64_tab[best].addr;
        u64 seg_end = seg_addr + vram64_tab[best].size;
        u64 new_end;
        a = vram64_align_up(seg_addr, align);
        new_end = a + size;
        vram64_tab[best].used = 1;
        vram64_tab[best].free = 0;
        vram64_tab[best].purgeable = 0;
        vram64_tab[best].addr = a;
        vram64_tab[best].size = size;
        if (a > seg_addr) /* on dolgu */
            vram64_add_free(seg_addr, a - seg_addr);
        if (new_end < seg_end) /* artakalan */
            vram64_add_free(new_end, seg_end - new_end);
    }
    vram64_cursor++;
    return a;
}

void vram64_free(u64 addr) {
    int i;
    if (!vram64_ready || !addr) return;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        if (vram64_tab[i].used && !vram64_tab[i].free &&
            vram64_tab[i].addr == addr) {
            vram64_tab[i].free = 1;
            vram64_tab[i].purgeable = 0;
            return;
        }
    }
}

/* Parcalanma: bos kesit sayisi / toplam kesit (0..100). */
int vram64_fragmentation(void) {
    int i, free_segs = 0, total = 0;
    if (!vram64_ready) return -1;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        if (!vram64_tab[i].used) continue;
        total++;
        if (vram64_tab[i].free) free_segs++;
    }
    if (!total) return 0;
    return (free_segs * 100) / total;
}

int vram64_mark_purgeable(u64 addr) {
    int i;
    if (!addr) return -1;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        if (vram64_tab[i].used && !vram64_tab[i].free &&
            vram64_tab[i].addr == addr) {
            vram64_tab[i].purgeable = 1;
            return 0;
        }
    }
    return -2;
}

int vram64_purge(void) {
    int i;
    u64 back = 0;
    if (!vram64_ready) return -1;
    for (i = 0; i < VRAM64_MAX_SEG; i++) {
        if (vram64_tab[i].used && !vram64_tab[i].free &&
            vram64_tab[i].purgeable) {
            back += vram64_tab[i].size;
            vram64_tab[i].free = 1;
            vram64_tab[i].purgeable = 0;
        }
    }
    return back > 0x7FFFFFFFULL ? 0x7FFFFFFF : (int)back;
}
