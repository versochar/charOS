#ifndef ADV_MEM_H
#define ADV_MEM_H

#include "arch/x86_64/longmode.h"

int advmem64_init(void);
int advmem64_alloc_node(int node, int order, u64 *out_phys);
int advmem64_map_huge(u64 virt, u64 phys, u64 flags, int level);
void advmem64_kasan_poison(const void *addr, u64 size);
int advmem64_kasan_check(const void *addr, u64 size);
int advmem64_reclaim(u64 target_pages, u64 *freed);

#endif
