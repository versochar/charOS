/* 36I: jemalloc tarzi API — slab64/align64 ustu bayrakli tahsis.
 * flags: dusuk 6 bit = lg_align (hizalama 2^n), JEM64_ZERO = sifirla.
 */
#include "arch/x86_64/longmode.h"

u64 jem64_nallocx(u64 size) {
    int cls = slab64_class_of(size);
    if (cls < 0) return 0;
    return slab64_class_size(cls);
}

void *jem64_mallocx(u64 size, int flags) {
    int lg = flags & 0x3F;
    u64 align = lg >= 64 ? 0 : (1ULL << lg);
    void *p;
    u64 i;
    if (lg > 11) return 0; /* >2K sinif disi */
    if (flags & JEM64_ZERO) {
        /* Hizalamali + sifirli: align uzerinden, sonra sifirla */
        if (lg) {
            p = align64_alloc(size, align);
            if (!p) return 0;
            for (i = 0; i < size; i++) ((unsigned char *)p)[i] = 0;
            return p;
        }
    }
    if (lg)
        return align64_alloc(size, align);
    p = slab64_alloc(size);
    if (p && (flags & JEM64_ZERO)) {
        for (i = 0; i < size; i++) ((unsigned char *)p)[i] = 0;
    }
    return p;
}

void jem64_dallocx(void *p, u64 size) {
    if (!p) return;
    /* Kayit defteri kesin soyluyor (sezgi yok): */
    if (align64_is_aligned(p)) {
        align64_free(p);
        return;
    }
    slab64_free(p, size);
}

void *jem64_rallocx(void *p, u64 oldsize, u64 size, int flags) {
    void *n;
    u64 copy, i;
    if (!p) return jem64_mallocx(size, flags);
    if (!size) {
        jem64_dallocx(p, oldsize);
        return 0;
    }
    n = jem64_mallocx(size, flags);
    if (!n) return 0;
    copy = oldsize < size ? oldsize : size;
    for (i = 0; i < copy; i++)
        ((unsigned char *)n)[i] = ((unsigned char *)p)[i];
    jem64_dallocx(p, oldsize);
    return n;
}

u64 jem64_xallocx(void *p, u64 oldsize, u64 size) {
    (void)p;
    if (jem64_nallocx(size) == jem64_nallocx(oldsize) && size >= oldsize)
        return size; /* ayni sinif: yerinde buyume */
    return oldsize;
}

u64 jem64_sallocx(u64 size) {
    return jem64_nallocx(size);
}
