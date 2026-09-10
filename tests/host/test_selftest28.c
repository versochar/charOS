/* 28.5: GERÇEK kernel/test/selftest.c testi (aynı dosya derlenir).
 * Calistirma: make test-selftest28
 */
#include <stdio.h>
#include "test/selftest.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static int t_ok(void) { return 0; }
static int t_fail(void) { return 1; }
static int run_order[4];
static int run_n = 0;
static int t_seq0(void) { run_order[run_n++] = 0; return 0; }
static int t_seq1(void) { run_order[run_n++] = 1; return 0; }

int main(void) {
    /* 28.3: kayıt kuralları */
    selftest_init();
    CHECK(selftest_count() == 0, "28.3 bos");
    CHECK(selftest_register("a", t_ok) == 0, "28.3 kaydet");
    CHECK(selftest_count() == 1, "28.3 sayac");
    CHECK(selftest_register("a", t_ok) == -1, "28.3 kopya red");
    CHECK(selftest_register(NULL, t_ok) == -1, "28.3 null-ad red");
    CHECK(selftest_register("", t_ok) == -1, "28.3 bos-ad red");
    CHECK(selftest_register("b", NULL) == -1, "28.3 null-fn red");

    /* 28.3: toplu koşu + fail sayımı */
    CHECK(selftest_register("f", t_fail) == 0, "28.3 fail-kayit");
    CHECK(selftest_run_all() == 1, "28.3 bir fail");

    /* 28.3: kayıt sırası korunur */
    selftest_init();
    run_n = 0;
    selftest_register("s0", t_seq0);
    selftest_register("s1", t_seq1);
    CHECK(selftest_run_all() == 0, "28.3 sira-pass");
    CHECK(run_n == 2 && run_order[0] == 0 && run_order[1] == 1, "28.3 sira");

    /* 28.9: tablo doluluğu deterministik */
    {
        selftest_init();
        int i;
        static char names[32][8];
        for (i = 0; i < 32; i++) {
            names[i][0] = 't';
            names[i][1] = (char)('0' + (i / 10));
            names[i][2] = (char)('0' + (i % 10));
            names[i][3] = '\0';
            if (selftest_register(names[i], t_ok) != 0) break;
        }
        CHECK(i == 32 && selftest_count() == 32, "28.9 tablo-dolu");
        CHECK(selftest_register("tasiyor", t_ok) == -1, "28.9 tasma red");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
