/* 35J: zone64/buddy64/reclaim64/oom64/notify64/balloon64/ksm64/
 *      hugeswap64/memtest64 host testi + swap stresi.
 * Calistirma: make test-swap64
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

static int notify_hits = 0;
static void notify_cb(int level) { (void)level; notify_hits++; }

static unsigned char memtest_buf[8192];

int main(void) {
    u64 f, z;
    int i;
    u64 slots[512];
    u64 out[8];

    pmm64_set_frame_mapper(test_mapper);
    pmm64_init();
    pmm64_add_region(0, 128ULL * 1024 * 1024);

    zone64_init();
    CHECK(zone64_total_count(ZONE64_LOW) == 256, "35A low boyutu");
    CHECK(zone64_total_count(ZONE64_DMA) == 4096 - 256, "35A dma boyutu");
    f = zone64_alloc(ZONE64_DMA);
    CHECK(f >= 0x100000 && f < 0x1000000, "35A dma araligi");
    z = zone64_alloc(ZONE64_NORMAL);
    CHECK(z >= 0x1000000, "35A normal araligi");
    zone64_free(f);
    zone64_free(z);
    CHECK(zone64_alloc(99) == 0, "35A gecersiz zone red");

    CHECK(buddy64_init(0, 4) == 0, "35B init");
    f = buddy64_alloc(2);
    CHECK(f == 0, "35B ilk blok tabanda");
    CHECK(buddy64_alloc(4) == 0, "35B buyuk blok tukendi");
    buddy64_free(f, 2);
    CHECK(buddy64_free_count(4) == 1, "35B birlesme");
    CHECK(buddy64_alloc(2) == 0, "35B yeniden bolme");

    CHECK(swap64_swapon_ramdisk(600) == 0, "32I swapon");
    reclaim64_init();
    for (i = 0; i < 10; i++) {
        f = pmm64_alloc_frame();
        memset(test_mapper(f), i + 1, 4096);
        reclaim64_add(f);
    }
    reclaim64_touch(pmm64_alloc_frame()); /* kayitsiz: sessiz */
    CHECK(reclaim64_evict(4, out) == 4, "35C 4 tahliye");
    /* Stres: kalan + yeni, slotlar bitene kadar */
    for (i = 0; i < 40; i++) {
        f = pmm64_alloc_frame();
        if (!f) break;
        memset(test_mapper(f), 0xAA, 4096);
        reclaim64_add(f);
    }
    CHECK(reclaim64_evict(60, 0) > 0, "35J swap stresi");

    CHECK(oom64_add_task(100, 500, 0) == 0, "35D gorev");
    CHECK(oom64_add_task(101, 200, 50) == 0, "35D adj gorev");
    CHECK(oom64_pick() == 101, "35D adj agirligi");
    CHECK(oom64_kill(101) == 0 && oom64_killed(101), "35D oldur");
    CHECK(oom64_pick() == 100, "35D olu atlanır");

    CHECK(notify64_register(notify_cb, 0) == 0, "35E kayit");
    notify_hits = 0;
    CHECK(notify64_poll() > 0 && notify_hits > 0, "35E ates");

    {
        u64 before = pmm64_free_frames();
        CHECK(balloon64_inflate(8) == 0, "35F sisme");
        CHECK(balloon64_pages() == 8, "35F sayac");
        CHECK(pmm64_free_frames() == before - 8, "35F pmm eksildi");
        CHECK(balloon64_deflate(8) == 0 && balloon64_pages() == 0,
              "35F inme");
        CHECK(pmm64_free_frames() == before, "35F iade");
    }

    ksm64_init();
    {
        u64 fa = pmm64_alloc_frame();
        u64 fb = pmm64_alloc_frame();
        memset(test_mapper(fa), 0x5A, 4096);
        memset(test_mapper(fb), 0x5A, 4096);
        CHECK(ksm64_scan(fa) == 0, "35G ilk kayit");
        CHECK(ksm64_scan(fb) == 1, "35G birlesme");
        CHECK(ksm64_shared() == 1 && ksm64_saved() == 1, "35G sayac");
    }

    {
        /* pmm araligi (128MB) DISINDA sentetik taban: ramdisk/pmm frame
         * numaralariyla aliasing olmamali */
        u64 huge = 0x10000000ULL; /* 2MB hizali, pmm disi */
        for (i = 0; i < 512; i++) {
            void *p = test_mapper(huge + (u64)i * 4096);
            memset(p, i & 0xFF, 4096);
        }
        CHECK(hugeswap64_out(huge, slots) == 0, "35H 2MB out");
        for (i = 0; i < 512; i++)
            memset(test_mapper(huge + (u64)i * 4096), 0, 4096);
        CHECK(hugeswap64_in(huge, slots) == 0, "35H 2MB in");
        CHECK(((unsigned char *)test_mapper(huge + 511ULL * 4096))[0] ==
              (511 & 0xFF), "35H icerik butunlugu");
    }

    CHECK(memtest64_run(memtest_buf, sizeof(memtest_buf)) == 0,
          "35I saglam bellek");
    CHECK(memtest64_run(0, 100) != 0, "35I null red");

    for (i = 0; i < map_n; i++) free(map_ptr[i]);
    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
