/* 33J: smp64/aptramp64/lapic_timer64/tlb64/wq64/aff64/numa64/rcu64/lf64 testi.
 * Calistirma: make test-smp64
 * (AP blob'u aptramp64.o ile linklenir; yalniz magic/boyut okunur.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

extern unsigned char aptramp64_blob[];
extern unsigned char aptramp64_end[];

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static u32 fake_lapic[1024];

static int wq_flag = 0;
static void wq_cb(void *arg) { wq_flag += (int)(u64)arg; }

static int rcu_flag = 0;
static void rcu_cb(void *arg) { rcu_flag += (int)(u64)arg; }

static unsigned char lf_pool[64 * 32];

int main(void) {
    u64 sz;
    int i;
    void *objs[64];

    memset(fake_lapic, 0, sizeof(fake_lapic));
    smp64_send_init(fake_lapic, 1);
    CHECK(fake_lapic[LAPIC_ICR_HI / 4] == (1U << 24), "33A INIT hedef");
    CHECK(fake_lapic[LAPIC_ICR_LO / 4] == ((5U << 8) | (1U << 15)),
          "33A INIT deassert");
    CHECK(smp64_start_ap(fake_lapic, 2, 0x08) == 0, "33A start_ap");
    CHECK(fake_lapic[LAPIC_ICR_HI / 4] == (2U << 24), "33A SIPI hedef");
    CHECK(fake_lapic[LAPIC_ICR_LO / 4] == ((6U << 8) | 0x08), "33A SIPI vektor");

    sz = aptramp64_size();
    CHECK(sz > 16 && sz < 1024, "33B blob boyutu");
    CHECK(aptramp64_check(aptramp64_blob, sz) == 0, "33B magic OK");
    CHECK(aptramp64_check("XXXX1234", 8) != 0, "33B kotu magic red");
    CHECK(aptramp64_check(aptramp64_blob, 4096) != 0, "33B asiri boyut red");

    memset(fake_lapic, 0, sizeof(fake_lapic));
    CHECK(lapic_timer64_calibrate(fake_lapic, 2400000000ULL, 10) == 150000,
          "33C kalibrasyon orani");
    lapic_timer64_start(fake_lapic, 0x40, 1000);
    CHECK(fake_lapic[LAPIC_TMR_DIV / 4] == 0x3, "33C divider");
    CHECK(fake_lapic[LAPIC_LVT_TMR / 4] == 0x40, "33C vektor");
    CHECK(fake_lapic[LAPIC_TMR_INIT / 4] == 1000, "33C sayac");

    tlb64_init();
    CHECK(tlb64_request(1) == 0 && tlb64_pending(1), "33D istek");
    CHECK(tlb64_send(fake_lapic, 3) == 0, "33D IPI gonder");
    CHECK(fake_lapic[LAPIC_ICR_HI / 4] == (3U << 24), "33D IPI hedef");
    CHECK(fake_lapic[LAPIC_ICR_LO / 4] == 0x50, "33D IPI vektor");
    tlb64_ack(1);
    CHECK(!tlb64_pending(1), "33D ack");

    wq64_init();
    wq_flag = 0;
    CHECK(wq64_enqueue(0, wq_cb, (void *)1) == 0, "33E kuyruk");
    CHECK(wq64_enqueue(0, wq_cb, (void *)2) == 0, "33E kuyruk2");
    CHECK(wq64_pending(0) == 2, "33E bekleyen");
    CHECK(wq64_run(0) == 2 && wq_flag == 3, "33E calistir");
    CHECK(wq64_pending(0) == 0, "33E bosaldi");

    CHECK(aff64_init(4) == 0, "33F init");
    CHECK(aff64_set(0, 0x5) == 0, "33F maske");
    CHECK(aff64_get(0) == 0x5, "33F oku");
    CHECK(aff64_set(1, 0) != 0, "33F bos maske red");
    CHECK(aff64_pick(0) == 0, "33F ilk secim");
    CHECK(aff64_pick(0) == 2, "33F yuk dengeleme");

    CHECK(numa64_init() == 0, "33G init");
    CHECK(numa64_add(0, 0x40000000ULL, 0) == 0, "33G dugum0");
    CHECK(numa64_add(0x40000000ULL, 0x40000000ULL, 1) == 0, "33G dugum1");
    CHECK(numa64_nodes() == 2, "33G dugum sayisi");
    CHECK(numa64_node_of(0x100) == 0, "33G adres dugum0");
    CHECK(numa64_node_of(0x50000000ULL) == 1, "33G adres dugum1");
    CHECK(numa64_node_of(0xFFFFFFFFULL) == 0, "33G varsayilan");

    rcu64_init();
    rcu_flag = 0;
    rcu64_read_lock();
    rcu64_call(rcu_cb, (void *)5);
    CHECK(rcu64_process() == 0 && !rcu_flag, "33H okuyucu bloklar");
    rcu64_read_unlock();
    rcu64_quiescent(0);
    CHECK(rcu64_process() == 1 && rcu_flag == 5, "33H grace sonrasi");

    lf64_init(lf_pool, 32, 64);
    for (i = 0; i < 64; i++) {
        objs[i] = lf64_alloc();
        if (!objs[i]) break;
    }
    CHECK(i == 64, "33I 64 nesne");
    CHECK(lf64_alloc() == 0, "33I tukendi");
    for (i = 0; i < 10; i++) lf64_free(objs[i]);
    for (i = 0; i < 10; i++) objs[i] = lf64_alloc();
    CHECK(objs[9] != 0, "33I geri donusum");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
