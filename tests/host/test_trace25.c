/* 25.5: GERÇEK kernel/core/trace.c testi (aynı dosya derlenir).
 * Calistirma: make test-trace25
 */
#include <stdio.h>
#include "core/trace.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static uint32_t got_level = 0;
static const char* got_tag = 0;
static const char* got_msg = 0;
static uint32_t be_hits = 0;
static void rec_be(uint32_t level, const char* tag, const char* msg) {
    got_level = level; got_tag = tag; got_msg = msg;
    be_hits++;
}

int main(void) {
    /* 25.3: eşik filtresi + sayaç */
    trace_init();
    trace_set_backend(rec_be);
    CHECK(trace_event(TRACE_DEBUG, "t", "d") == -1 && be_hits == 0,
          "25.3 debug esik-alti duser");
    CHECK(trace_event(TRACE_INFO, "t", "i") == 0 && be_hits == 1,
          "25.3 info iletilir");
    CHECK(trace_count(TRACE_INFO) == 1 && trace_count(TRACE_DEBUG) == 0,
          "25.3 sayac");
    CHECK(got_level == TRACE_INFO && got_tag[0] == 't' && got_msg[0] == 'i',
          "25.3 arka-uc arguman");

    /* 25.3: parametre hataları */
    CHECK(trace_event(99, "t", "m") == -1, "25.3 seviye-aralik");
    CHECK(trace_event(TRACE_INFO, 0, "m") == -1, "25.3 null-tag");
    CHECK(trace_event(TRACE_INFO, "t", 0) == -1, "25.3 null-msg");
    CHECK(trace_count(99) == 0, "25.3 sayac-aralik");

    /* 25.3: eşik değiştirme */
    trace_set_level(TRACE_ERROR);
    CHECK(trace_event(TRACE_WARN, "t", "w") == -1, "25.3 warn duser");
    CHECK(trace_event(TRACE_ERROR, "t", "e") == 0, "25.3 error gecer");
    trace_set_level(99); /* geçersiz eşik: değişmez */
    CHECK(trace_event(TRACE_WARN, "t", "w") == -1, "25.3 esik korunur");

    /* 25.3: arka uçsuz düşer */
    trace_init();
    CHECK(trace_event(TRACE_FATAL, "t", "f") == -1, "25.3 backendsiz duser");

    /* 25.9: sayaç determinizmi */
    trace_init();
    trace_set_backend(rec_be);
    trace_event(TRACE_WARN, "a", "1");
    trace_event(TRACE_WARN, "a", "2");
    CHECK(trace_count(TRACE_WARN) == 2 && be_hits == 4, "25.9 deterministik");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
