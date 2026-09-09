/* 32J: pmm64/slab64/demand64/cow64/kaslr64/huge64/swap64/pressure64 host testi.
 * Calistirma: make test-mem64  (el ile: asagidaki komut; -iquote sart)
 *   gcc -iquote include kernel/arch/x86_64/pmm64.c kernel/arch/x86_64/slab64.c kernel/arch/x86_64/demand64.c kernel/arch/x86_64/cow64.c kernel/arch/x86_64/kaslr64.c kernel/arch/x86_64/percpu_pt.c kernel/arch/x86_64/huge64.c kernel/arch/x86_64/swap64.c kernel/arch/x86_64/pressure64.c tests/host/test_mem64.c -o /tmp/test_mem64 && /tmp/test_mem64
 *   NOT: duz -I include kullanma; kernel include/string.h host header'i gölgeler.
 * Frame->pointer eslemesi test mapper ile (gercek kernel64'te identity/paging).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

#define MAXMAP 512
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
    u64 f1, f2, total, free0, nf;
    void *a, *b, *c;
    u64 fr, out;
    int i;
    u64 slide;
    u64 *pml4, *pdp, *pool;
    u64 slot;
    unsigned char page[4096], back[4096];

    pmm64_set_frame_mapper(test_mapper);
    pmm64_init();
    pmm64_add_region(0, 64ULL * 1024 * 1024);
    pmm64_reserve(0, 1024 * 1024);
    total = pmm64_total_frames();
    free0 = pmm64_free_frames();
    CHECK(total == 16384 - 1, "32A toplam frame (frame0 ayrik)");
    CHECK(free0 == total - 255, "32A bos frame (frame0 zaten ayrikti)");

    f1 = pmm64_alloc_frame();
    f2 = pmm64_alloc_frame();
    CHECK(f1 && f2 && f1 != f2, "32A alloc");
    CHECK(pmm64_free_frames() == free0 - 2, "32A sayac");
    pmm64_free_frame(f1);
    CHECK(pmm64_free_frames() == free0 - 1, "32A free");

    a = slab64_alloc(13);
    b = slab64_alloc(2000);
    c = slab64_alloc(8);
    CHECK(a && b && c, "32B alloc siniflar");
    slab64_free(a, 13);
    slab64_free(b, 2000);
    slab64_free(c, 8);
    a = slab64_alloc(13);
    CHECK(a != 0, "32B yeniden alloc");

    CHECK(demand64_add(0x10000000ULL, 0x1000000ULL, D64_READ | D64_WRITE) == 0,
          "32C bolge ekle");
    CHECK(demand64_fault(0x10001000ULL, 0, &fr) == 0 && fr, "32C fault cozum");
    CHECK(demand64_fault(0x20000000ULL, 0, &fr) == -1, "32C kayitsiz red");
    CHECK(demand64_fault(0x10001000ULL, 1, &fr) == -2, "32C protection red");

    cow64_retain(fr);
    cow64_retain(fr);
    CHECK(cow64_is_shared(fr), "32D paylasim");
    CHECK(cow64_resolve(fr, &out) == 0 && out != fr, "32D kopyala-yaz");
    CHECK(!cow64_is_shared(fr), "32D cozumu sonrasi tekil");
    CHECK(cow64_release(fr) == 0, "32D release");
    CHECK(cow64_release(fr) == 0, "32D release sifir");

    nf = pmm64_free_frames();
    slide = kaslr64_slide(0x123456789ABCDEF0ULL, 2ULL * 1024 * 1024);
    CHECK((slide & 0x1FFFFFULL) == 0 && slide < (1ULL << 30), "32E slide hiza/aralik");
    CHECK(kaslr64_verify(0x1000000ULL + slide, 0x1000000ULL, slide) == 0,
          "32E verify");
    (void)nf;

    posix_memalign((void **)&pml4, 4096, 4096);
    posix_memalign((void **)&pdp, 4096, 4096);
    posix_memalign((void **)&pool, 4096, 2 * 512 * 8);
    memset(pml4, 0, 4096);
    CHECK(paging64_build_identity(pml4, pdp, pool, 2,
          16ULL * 1024 * 1024, PAGE_RW64) == 0, "32G identity kur");
    CHECK((pml4[0] & PAGE_PRESENT64) && (pdp[0] & PAGE_PRESENT64),
          "32G tablo zinciri");
    CHECK((pool[0] & (PAGE_PS64 | PAGE_PRESENT64)) ==
          (PAGE_PS64 | PAGE_PRESENT64), "32G 2MB PS");
    paging64_map_1gb_at(pdp, 1, 0x40000000ULL, PAGE_RW64);
    CHECK((pdp[1] & (PAGE_PS64 | PAGE_PRESENT64)) ==
          (PAGE_PS64 | PAGE_PRESENT64), "32G 1GB PS");

    CHECK(swap64_swapon_ramdisk(16) == 0, "32I ramdisk swapon");
    for (i = 0; i < 4096; i++) page[i] = (unsigned char)(i & 0xFF);
    CHECK(swap64_alloc_slot(&slot) == 0, "32H slot alloc");
    CHECK(swap64_out(slot, page) == 0, "32H swap out");
    memset(back, 0, sizeof(back));
    CHECK(swap64_in(slot, back) == 0 &&
          memcmp(page, back, sizeof(page)) == 0, "32H swap in/out");
    swap64_free_slot(slot);
    CHECK(swap64_alloc_slot(&slot) == 0, "32H slot yeniden");

    CHECK(pressure64_level() >= 0 && pressure64_level() <= 100,
          "32J seviye araligi");
    printf("32J basinc seviyesi: %d (reclaim=%d)\n",
           pressure64_level(), pressure64_should_reclaim());

    for (i = 0; i < map_n; i++) free(map_ptr[i]);
    free(pml4);
    free(pdp);
    free(pool);
    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
