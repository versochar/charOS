/* 33G: NUMA topology — proximity girdileri + dugum cozumu.
 * Gercek SRAT ayrıştırma 34x ACPI ile dolar; API simdiden sabit.
 */
#include "arch/x86_64/longmode.h"

#define NUMA64_MAX_ENTRIES 16

static struct numa64_entry numa64_tab[NUMA64_MAX_ENTRIES];
static int numa64_count = 0;
static int numa64_nnodes = 1; /* node 0 her zaman var */

int numa64_init(void) {
    int i;
    for (i = 0; i < NUMA64_MAX_ENTRIES; i++) {
        numa64_tab[i].base = 0;
        numa64_tab[i].len = 0;
        numa64_tab[i].node = 0;
        numa64_tab[i].reserved = 0;
    }
    numa64_count = 0;
    numa64_nnodes = 1;
    return 0;
}

int numa64_add(u64 base, u64 len, u32 node) {
    if (!len || node >= NUMA64_MAX_NODES) return -1;
    if (numa64_count >= NUMA64_MAX_ENTRIES) return -2;
    numa64_tab[numa64_count].base = base;
    numa64_tab[numa64_count].len = len;
    numa64_tab[numa64_count].node = node;
    numa64_tab[numa64_count].reserved = 0;
    numa64_count++;
    if ((int)node + 1 > numa64_nnodes) numa64_nnodes = (int)node + 1;
    return 0;
}

u32 numa64_node_of(u64 addr) {
    int i;
    for (i = 0; i < numa64_count; i++) {
        if (addr >= numa64_tab[i].base &&
            addr < numa64_tab[i].base + numa64_tab[i].len)
            return numa64_tab[i].node;
    }
    return 0; /* varsayilan dugum */
}

int numa64_nodes(void) { return numa64_nnodes; }
