/* 36G: 16-bayt (genel 2^n) hizalama — asiri-tahsis + kaydirma.
 * Ham pointer hizali bloktan once saklanir; ayni sinif kosulu:
 * size + align <= 2048 (slab64 tavani).
 */
#include "arch/x86_64/longmode.h"

u64 align64_up(u64 v, u64 a) {
    if (!align64_is_pow2(a)) return v;
    return (v + a - 1) & ~(a - 1);
}

u64 align64_down(u64 v, u64 a) {
    if (!align64_is_pow2(a)) return v;
    return v & ~(a - 1);
}

int align64_is_pow2(u64 a) {
    return a && !(a & (a - 1));
}

/* Baslik: [ham ptr (8B)][ham boyut u64 (8B)] hizali kullanici oncesi.
 * Verilen hizali pointer kaydi (jem64 ayirimi icin). */
#define ALIGN64_MAX 128
static void *align64_registry[ALIGN64_MAX];

static void align64_reg_add(void *p) {
    int i;
    for (i = 0; i < ALIGN64_MAX; i++) {
        if (!align64_registry[i]) {
            align64_registry[i] = p;
            return;
        }
    }
}

static void align64_reg_del(void *p) {
    int i;
    for (i = 0; i < ALIGN64_MAX; i++) {
        if (align64_registry[i] == p) {
            align64_registry[i] = 0;
            return;
        }
    }
}

int align64_is_aligned(const void *p) {
    int i;
    for (i = 0; i < ALIGN64_MAX; i++)
        if (align64_registry[i] == p) return 1;
    return 0;
}

void *align64_alloc(u64 size, u64 align) {
    unsigned char *raw;
    u64 addr, aligned, total;
    void *out;
    if (!size || !align64_is_pow2(align)) return 0;
    if (align < 16) align = 16;
    total = size + align;
    if (total > 2048) return 0;
    raw = (unsigned char *)slab64_alloc(total);
    if (!raw) return 0;
    addr = (u64)raw;
    aligned = align64_up(addr + 16, align);
    *(void **)(aligned - 16) = raw;
    *(u64 *)(aligned - 8) = total;
    out = (void *)aligned;
    align64_reg_add(out);
    return out;
}

void align64_free(void *p) {
    void *raw;
    u64 total;
    if (!p) return;
    raw = *(void **)((unsigned char *)p - 16);
    total = *(u64 *)((unsigned char *)p - 8);
    align64_reg_del(p);
    slab64_free(raw, total);
}
