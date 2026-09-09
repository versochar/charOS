/* 32B: slab64 — 8B..2K boyut siniflari, pmm64 ustu.
 * Her slab bir frame'dir; bos nesneler frame icinde bagli listedir.
 * Saf C, asm yok (host testi uygun).
 */
#include "arch/x86_64/longmode.h"

#define SLAB64_MAGIC 0x51AB6401UL
#define SLAB64_CLASSES 9

static const u64 slab64_sizes[SLAB64_CLASSES] =
    {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

struct slab64 {
    u32 magic;
    u32 cls;
    u64 free;
    u64 total;   /* 36B: shrink tam-bos karari icin */
    u64 frame;   /* 36B: iade edilecek frame numarasi */
    void *head;
    struct slab64 *next;
};

static struct slab64 *slab64_caches[SLAB64_CLASSES];

static int cls_for(u64 size) {
    int i;
    for (i = 0; i < SLAB64_CLASSES; i++)
        if (size <= slab64_sizes[i] && size > 0) return i;
    return -1;
}

static struct slab64 *slab64_new(int cls) {
    u64 frame = pmm64_alloc_frame();
    struct slab64 *s;
    u64 objsz = slab64_sizes[cls];
    u64 avail = PMM64_FRAME - sizeof(struct slab64);
    u64 n = avail / objsz;
    u64 i;
    unsigned char *p;
    if (!frame) return 0;
    s = (struct slab64 *)pmm64_frame_ptr(frame);
    s->magic = (u32)SLAB64_MAGIC;
    s->cls = (u32)cls;
    s->free = n;
    s->total = n;
    s->frame = frame;
    s->head = 0;
    s->next = slab64_caches[cls];
    p = (unsigned char *)s + sizeof(struct slab64);
    /* Nesneleri ters sirayla diz (ilk alloc dusuk adres) */
    for (i = 0; i < n; i++) {
        void **slot = (void **)(p + (n - 1 - i) * objsz);
        *slot = s->head;
        s->head = slot;
    }
    slab64_caches[cls] = s;
    return s;
}

void *slab64_alloc(u64 size) {
    int cls = cls_for(size);
    struct slab64 *s;
    void *obj;
    if (cls < 0) return 0;
    s = slab64_caches[cls];
    while (s && !s->head) s = s->next;
    if (!s) {
        s = slab64_new(cls);
        if (!s) return 0;
    }
    obj = s->head;
    s->head = *(void **)obj;
    s->free--;
    return obj;
}

void slab64_free(void *p, u64 size) {
    int cls = cls_for(size);
    struct slab64 *s;
    u64 objsz;
    if (!p || cls < 0) return;
    objsz = slab64_sizes[cls];
    /* Hangi slab'a ait: frame tabanini bul */
    for (s = slab64_caches[cls]; s; s = s->next) {
        u64 base = (u64)s;
        u64 addr = (u64)p;
        if (s->magic != (u32)SLAB64_MAGIC) continue;
        if (addr > base && addr < base + PMM64_FRAME &&
            ((addr - base - sizeof(struct slab64)) % objsz) == 0) {
            *(void **)p = s->head;
            s->head = p;
            s->free++;
            return;
        }
    }
}

/* 36B destegi: sinif sorgu */
int slab64_class_of(u64 size) { return cls_for(size); }

u64 slab64_class_size(int cls) {
    if (cls < 0 || cls >= SLAB64_CLASSES) return 0;
    return slab64_sizes[cls];
}

/* 36B: tam bos slab'lari pmm'ye iade et. Her sinifta en az bir yedek
 * slab birakilir (thrashing onlenir). Donus: iade adedi. */
int slab64_shrink(void) {
    int cls, freed = 0;
    for (cls = 0; cls < SLAB64_CLASSES; cls++) {
        struct slab64 *s = slab64_caches[cls];
        struct slab64 *prev = 0;
        int spare_kept = 0;
        while (s) {
            struct slab64 *next = s->next;
            if (s->magic == (u32)SLAB64_MAGIC && s->free == s->total &&
                spare_kept) {
                if (prev)
                    prev->next = next;
                else
                    slab64_caches[cls] = next;
                pmm64_free_frame(s->frame);
                freed++;
            } else {
                if (s->magic == (u32)SLAB64_MAGIC && s->free == s->total)
                    spare_kept = 1;
                prev = s;
            }
            s = next;
        }
    }
    return freed;
}
