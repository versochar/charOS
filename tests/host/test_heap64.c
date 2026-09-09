/* 36J: cachecolor64/shrink/profile64/leak64/kasan64/guard64/align64/
 *      arena64/jem64 host testi + heap stresi.
 * Calistirma: make test-heap64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

#define MAXMAP 2048
static u64 map_frame[MAXMAP];
static void *map_ptr[MAXMAP];
static int map_n = 0;

static void *test_mapper(u64 frame) {
    int i;
    void *p;
    for (i = 0; i < map_n; i++)
        if (map_frame[i] == frame) return map_ptr[i];
    p = malloc(4096);
    memset(p, 0, 4096);
    if (map_n < MAXMAP) {
        map_frame[map_n] = frame;
        map_ptr[map_n] = p;
        map_n++;
    }
    return p;
}

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int i;
    u64 a, b, c, d;

    pmm64_set_frame_mapper(test_mapper);
    pmm64_init();
    pmm64_add_region(0, 128ULL * 1024 * 1024);

    a = cachecolor64_next(64);
    b = cachecolor64_next(64);
    c = cachecolor64_next(64);
    d = cachecolor64_next(64);
    CHECK(a < 64 && b < 64 && c < 64 && d < 64, "36A aralik");
    CHECK(a != b && b != c && c != d, "36A donusum");
    CHECK(cachecolor64_next(8) < 8, "36A kucuk sinif");

    {
        /* 126 nesne = 1 slab (32B); 200 nesne = 2 slab -> shrink 1 iade */
        void *objs[200];
        int n = 0;
        u64 f0 = pmm64_free_frames();
        for (i = 0; i < 200; i++) {
            objs[n] = slab64_alloc(32);
            if (!objs[n]) break;
            n++;
        }
        CHECK(n == 200, "36B 2 slab doldu");
        for (i = 0; i < n; i++) slab64_free(objs[i], 32);
        CHECK(slab64_shrink() == 1, "36B 1 iade + 1 yedek");
        CHECK(pmm64_free_frames() == f0 - 1, "36B pmm dengesi");
    }

    {
        void *p1 = profile64_alloc(100);
        void *p2 = profile64_alloc(100);
        u64 allocs = 0, bytes = 0, peak = 0;
        int cls = slab64_class_of(100);
        CHECK(p1 && p2, "36C alloc");
        profile64_free(p1, 100);
        CHECK(profile64_get(cls, &allocs, &bytes, &peak) == 0 &&
              allocs == 2 && bytes == 100 && peak == 200, "36C sayac");
        profile64_free(p2, 100);
        CHECK(profile64_get(99, 0, 0, 0) != 0, "36C gecersiz sinif red");
    }

    {
        void *l1 = slab64_alloc(16);
        void *l2 = slab64_alloc(16);
        void *rp;
        u64 sz;
        CHECK(leak64_add(l1, 16) == 0, "36D ekle");
        CHECK(leak64_add(l2, 16) == 0, "36D ekle2");
        CHECK(leak64_count() == 2, "36D sayac");
        leak64_remove(l1);
        CHECK(leak64_count() == 1, "36D cikar");
        CHECK(leak64_get(0, &rp, &sz) != 0, "36D bos slot red");
        slab64_free(l1, 16);
        slab64_free(l2, 16);
        leak64_remove(l2);
        CHECK(leak64_count() == 0, "36D temiz");
    }

    {
        unsigned char *k = (unsigned char *)kasan64_alloc(100);
        CHECK(k != 0, "36E alloc");
        CHECK(kasan64_check(k, 100) == 0, "36E temiz");
        k[100] = 0xFF; /* sag redzone ihlali */
        CHECK(kasan64_check(k, 100) > 0, "36E ihlal yakalama");
        k[100] = 0xAB;
        CHECK(kasan64_check(k, 100) == 0, "36E onarim");
        kasan64_free(k, 100);
        CHECK(kasan64_alloc(4096) == 0, "36E asiri red");
    }

    {
        u64 base = guard64_alloc_pages(2);
        unsigned char *g;
        CHECK(base != 0, "36F tahsis");
        CHECK(guard64_check(base) == 0, "36F saglam");
        g = (unsigned char *)test_mapper(base + 2 * 4096);
        g[0] ^= 0xFF; /* bekci ihlali */
        CHECK(guard64_check(base) > 0, "36F ihlal");
        g[0] ^= 0xFF;
        guard64_free(base);
        CHECK(guard64_check(base) != 0, "36F free sonrasi kayitsiz");
    }

    {
        void *p = align64_alloc(100, 16);
        CHECK(p && ((u64)p % 16) == 0, "36G hizali");
        CHECK(align64_up(100, 16) == 112, "36G up");
        CHECK(align64_down(100, 16) == 96, "36G down");
        CHECK(!align64_is_pow2(24) && align64_is_pow2(32), "36G pow2");
        CHECK(align64_is_aligned(p), "36G kayit");
        memset(p, 0xCC, 100);
        align64_free(p);
        CHECK(!align64_is_aligned(p), "36G kayit silindi");
        CHECK(align64_alloc(100, 24) == 0, "36G pow2 red");
    }

    {
        void *x = arena64_alloc(0, 100);
        void *y = arena64_alloc(1, 100);
        CHECK(x && y, "36H per-cpu alloc");
        memset(x, 1, 100);
        memset(y, 2, 100);
        arena64_free(0, x, 100);
        arena64_free(1, y, 100);
        x = arena64_alloc(0, 100);
        CHECK(x != 0, "36H dergi geri");
        CHECK(arena64_alloc(99, 100) == 0, "36H gecersiz cpu red");
    }

    {
        void *m;
        u64 nc;
        CHECK(jem64_nallocx(100) == 128, "36I nallocx");
        m = jem64_mallocx(100, 0);
        CHECK(m != 0, "36I mallocx");
        CHECK(jem64_sallocx(100) == 128, "36I sallocx");
        CHECK(jem64_xallocx(m, 100, 120) == 120, "36I yerinde buyume");
        CHECK(jem64_xallocx(m, 100, 500) == 100, "36I sinif disi");
        {
            void *r = jem64_rallocx(m, 100, 200, 0);
            CHECK(r != 0, "36I rallocx");
            jem64_dallocx(r, 200);
        }
        {
            void *z = jem64_mallocx(64, JEM64_ZERO);
            int ok = 1, k;
            for (k = 0; k < 64; k++)
                if (((unsigned char *)z)[k]) ok = 0;
            CHECK(ok, "36I sifirli");
            jem64_dallocx(z, 64);
        }
        {
            void *h = jem64_mallocx(64, 4); /* 16B hizali */
            CHECK(h && ((u64)h % 16) == 0, "36I hizali mallocx");
            jem64_dallocx(h, 64);
        }
        nc = jem64_nallocx(5000);
        CHECK(nc == 0, "36I asiri red");
    }

    /* Stres: karisik alloc/free dongusu (boyutlar eslesik) + shrink */
    {
        struct { void *p; u64 sz; } v[128];
        int n = 0;
        for (i = 0; i < 400; i++) {
            if (n < 128 && (i % 3 != 2)) {
                u64 sz = (u64)((i * 37) % 1500 + 1);
                void *p = slab64_alloc(sz);
                if (p) {
                    v[n].p = p;
                    v[n].sz = sz;
                    n++;
                }
            } else if (n > 0) {
                n--;
                slab64_free(v[n].p, v[n].sz);
            }
        }
        while (n > 0) {
            n--;
            slab64_free(v[n].p, v[n].sz);
        }
        CHECK(slab64_shrink() >= 1, "36J stres shrink");
    }

    for (i = 0; i < map_n; i++) free(map_ptr[i]);
    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
