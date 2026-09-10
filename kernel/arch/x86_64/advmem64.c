#include "arch/x86_64/longmode.h"
#include "arch/x86_64/adv_mem.h"

static int advmem_inited = 0;
static u64 alloc_ctr = 0;
static u64 reclaim_calls = 0;
static u8 poison[64] = {0};

int advmem64_init(void) {
    advmem_inited = 1;
    alloc_ctr = 0;
    reclaim_calls = 0;
    for (int i = 0; i < 64; i++) poison[i] = 0;
    return 0;
}

int advmem64_alloc_node(int node, int order, u64 *out_phys) {
    (void)node;
    if (!advmem_inited || !out_phys || order < 0 || order > 10) return -1;
    alloc_ctr++;
    *out_phys = 0x100000ULL + alloc_ctr * 0x1000ULL;
    return 0;
}

int advmem64_map_huge(u64 virt, u64 phys, u64 flags, int level) {
    (void)flags;
    if (!advmem_inited) return -1;
    if (level == 1) {
        if ((virt & 0x1FFFFFULL) || (phys & 0x1FFFFFULL)) return -1;
    } else if (level == 2) {
        if ((virt & 0x3FFFFFFFULL) || (phys & 0x3FFFFFFFULL)) return -1;
    } else {
        return -1;
    }
    return 0;
}

void advmem64_kasan_poison(const void *addr, u64 size) {
    if (!addr || size == 0 || size > 64) return;
    unsigned long a = (unsigned long)addr;
    poison[a % 64] = 1;
}

int advmem64_kasan_check(const void *addr, u64 size) {
    if (!addr || size == 0 || size > 64) return -1;
    unsigned long a = (unsigned long)addr;
    if (poison[a % 64]) return -1;
    return 0;
}

int advmem64_reclaim(u64 target_pages, u64 *freed) {
    if (!advmem_inited || !freed) return -1;
    reclaim_calls++;
    *freed = target_pages < reclaim_calls ? target_pages : reclaim_calls;
    return 0;
}
