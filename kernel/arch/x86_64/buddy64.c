/* 35B: buddy allocator — order 0..9 (4K..2MB bloklar).
 * Yonetim bitmapleri statik; blok bellegi pmm64'ten ayri arena olarak verilir
 * (buddy64_init taban+duzey). Saf C, host testi uygun.
 */
#include "arch/x86_64/longmode.h"

#define BUDDY64_MAX_ORDER 9
#define BUDDY64_MAX_BLOCKS (PMM64_MAX_MEMORY / PMM64_FRAME) /* order0 */

/* Her order icin: 1=bos(bedava), 0=dolu/bolunmus. Order o'da N blok. */
#define BUDDY64_WORDS(o) ((BUDDY64_MAX_BLOCKS >> (o)) + 63) / 64
static u64 buddy64_free_map[10][8192];
static u64 buddy64_base = 0; /* arena taban frame adresi */
static int buddy64_orders = 0;
static int buddy64_ready = 0;

static void bset(int o, u64 b) { buddy64_free_map[o][b >> 6] |= (1ULL << (b & 63)); }
static void bclear(int o, u64 b) { buddy64_free_map[o][b >> 6] &= ~(1ULL << (b & 63)); }
static int bget(int o, u64 b) {
    return (int)((buddy64_free_map[o][b >> 6] >> (b & 63)) & 1ULL);
}
static u64 bcount(int o) { return BUDDY64_MAX_BLOCKS >> (o); }

/* buddy64_init: [base, base + 2^max_order frame) arenasini order max'ta acar.
 * Diger orderlar bos baslar; split ile dolar. */
int buddy64_init(u64 base_frame, int max_order) {
    int o;
    u64 i;
    if (max_order < 0 || max_order > BUDDY64_MAX_ORDER) return -1;
    if (base_frame % (1ULL << max_order)) return -2; /* hizali olmali */
    for (o = 0; o <= BUDDY64_MAX_ORDER; o++)
        for (i = 0; i < 8192; i++) buddy64_free_map[o][i] = 0;
    buddy64_base = base_frame;
    buddy64_orders = max_order;
    /* Tek arena: 1x 2^max_order blok (skeleton); split ile alt orderlar dolar */
    bset(max_order, 0);
    buddy64_ready = 1;
    return 0;
}

u64 buddy64_alloc(int order) {
    int o;
    u64 b, i;
    if (!buddy64_ready || order < 0 || order > buddy64_orders) return 0;
    /* Uygun veya buyuk bos blok ara */
    for (o = order; o <= buddy64_orders; o++) {
        for (b = 0; b < bcount(o); b++) {
            if (!bget(o, b)) continue;
            /* Bulundu: hedef order'a kadar bol */
            bclear(o, b);
            while (o > order) {
                o--;
                b = b * 2;
                bset(o, b + 1); /* sag kardes bosa */
                /* sol kardes (b) ile devam */
            }
            i = buddy64_base / PMM64_FRAME + b * (1ULL << order);
            return i * PMM64_FRAME;
        }
    }
    return 0;
}

void buddy64_free(u64 frame, int order) {
    u64 b, buddy;
    int o;
    if (!buddy64_ready || order < 0 || order > buddy64_orders) return;
    if (frame < buddy64_base) return;
    b = (frame / PMM64_FRAME - buddy64_base / PMM64_FRAME) >> order;
    if (b >= bcount(order)) return;
    o = order;
    /* Birkestir: kardes bossa bir ust order'a cik */
    while (o < buddy64_orders) {
        buddy = b ^ 1;
        if (buddy >= bcount(o) || !bget(o, buddy)) break;
        bclear(o, buddy);
        b = b / 2;
        o++;
    }
    bset(o, b);
}

u64 buddy64_free_count(int order) {
    u64 b, n = 0;
    if (!buddy64_ready || order < 0 || order > buddy64_orders) return 0;
    for (b = 0; b < bcount(order); b++)
        if (bget(order, b)) n++;
    return n;
}
