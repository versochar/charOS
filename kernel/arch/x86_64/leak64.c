/* 36D: leak detector — acik tahsis tablosu + cagiran adresi. */
#include "arch/x86_64/longmode.h"

#define LEAK64_MAX 256

struct leak64_entry {
    int used;
    void *ptr;
    u64 size;
    void *caller;
};

static struct leak64_entry leak64_tab[LEAK64_MAX];

int leak64_add(void *ptr, u64 size) {
    int i;
    if (!ptr) return -1;
    for (i = 0; i < LEAK64_MAX; i++) {
        if (!leak64_tab[i].used) {
            leak64_tab[i].used = 1;
            leak64_tab[i].ptr = ptr;
            leak64_tab[i].size = size;
            leak64_tab[i].caller = __builtin_return_address(0);
            return 0;
        }
    }
    return -2; /* tablo dolu */
}

void leak64_remove(void *ptr) {
    int i;
    if (!ptr) return;
    for (i = 0; i < LEAK64_MAX; i++) {
        if (leak64_tab[i].used && leak64_tab[i].ptr == ptr) {
            leak64_tab[i].used = 0;
            return;
        }
    }
}

int leak64_count(void) {
    int i, n = 0;
    for (i = 0; i < LEAK64_MAX; i++)
        if (leak64_tab[i].used) n++;
    return n;
}

int leak64_get(int i, void **ptr, u64 *size) {
    if (i < 0 || i >= LEAK64_MAX || !leak64_tab[i].used) return -1;
    if (ptr) *ptr = leak64_tab[i].ptr;
    if (size) *size = leak64_tab[i].size;
    return 0;
}
