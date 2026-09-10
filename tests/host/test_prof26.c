/* 26.5: GERÇEK kernel/core/prof.c testi (aynı dosya derlenir).
 * Tick kaynağı sahte sayaçla enjekte edilir (deterministik).
 * Calistirma: make test-prof26
 */
#include <stdio.h>
#include "core/prof.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static uint64_t mock_tick = 0;
static uint64_t mock_fn(void) { return mock_tick; }

int main(void) {
    uint32_t cnt = 0;
    uint64_t tot = 0, mn = 0, mx = 0;

    /* 26.3: temel ölçüm */
    prof_init();
    prof_set_tick(mock_fn);
    mock_tick = 1000;
    CHECK(prof_begin(0) == 0, "26.3 begin");
    mock_tick = 1100;
    CHECK(prof_end(0) == 0, "26.3 end");
    CHECK(prof_read(0, &cnt, &tot, &mn, &mx) == 0 &&
          cnt == 1 && tot == 100 && mn == 100 && mx == 100, "26.3 min=max=100");

    /* 26.3: min/max birikimi */
    mock_tick = 2000;
    prof_begin(0);
    mock_tick = 2200;
    prof_end(0);
    CHECK(prof_read(0, &cnt, &tot, &mn, &mx) == 0 &&
          cnt == 2 && tot == 300 && mn == 100 && mx == 200, "26.3 birikim");

    /* 26.3: u64 sarmalı güvenli (mock 2^64-10 -> 20) */
    mock_tick = (uint64_t)-10;
    prof_begin(1);
    mock_tick = 20;
    prof_end(1);
    CHECK(prof_read(1, &cnt, &tot, &mn, &mx) == 0 && tot == 30, "26.3 sarmal");

    /* 26.3: hata yolları */
    CHECK(prof_begin(99) == -1, "26.3 id-aralik");
    CHECK(prof_end(2) == -1, "26.3 acik-olmayan");
    prof_init(); /* tick sıfırlanır */
    CHECK(prof_begin(0) == -1, "26.3 ticksiz red");
    prof_set_tick(mock_fn);
    CHECK(prof_begin(0) == 0, "26.3 tekrar");
    CHECK(prof_begin(0) == -1, "26.3 ic-ice red");
    CHECK(prof_end(0) == 0, "26.3 kapat");
    CHECK(prof_read(0, &cnt, &tot, &mn, &mx) == 0 && cnt == 1, "26.3 sayac");
    CHECK(prof_read(0, 0, &tot, &mn, &mx) == -1, "26.3 null red");
    CHECK(prof_read(99, &cnt, &tot, &mn, &mx) == -1, "26.3 read-aralik");
    CHECK(prof_reset(0) == 0, "26.3 reset");
    CHECK(prof_read(0, &cnt, &tot, &mn, &mx) == 0 && cnt == 0 && tot == 0,
          "26.3 sifirlandi");
    CHECK(prof_reset(99) == -1, "26.3 reset-aralik");

    /* 26.9: deterministik tekrar */
    prof_init();
    prof_set_tick(mock_fn);
    mock_tick = 0;
    prof_begin(3); mock_tick = 50; prof_end(3);
    prof_begin(3); mock_tick = 150; prof_end(3);
    CHECK(prof_read(3, &cnt, &tot, &mn, &mx) == 0 &&
          cnt == 2 && tot == 150 && mn == 50 && mx == 100, "26.9 deterministik");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
