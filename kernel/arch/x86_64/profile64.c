/* 36C: malloc profiling — sinif bazinda sayaclar (alloc/free/bytes/peak). */
#include "arch/x86_64/longmode.h"

#define PROFILE64_CLASSES 9

struct profile64_stat {
    u64 allocs;
    u64 frees;
    u64 bytes; /* halen disarida */
    u64 peak;
};

static struct profile64_stat profile64_tab[PROFILE64_CLASSES];

void *profile64_alloc(u64 size) {
    int cls = slab64_class_of(size);
    void *p;
    if (cls < 0) return 0;
    p = slab64_alloc(size);
    if (!p) return 0;
    profile64_tab[cls].allocs++;
    profile64_tab[cls].bytes += size;
    if (profile64_tab[cls].bytes > profile64_tab[cls].peak)
        profile64_tab[cls].peak = profile64_tab[cls].bytes;
    return p;
}

void profile64_free(void *p, u64 size) {
    int cls = slab64_class_of(size);
    if (!p || cls < 0) return;
    slab64_free(p, size);
    profile64_tab[cls].frees++;
    if (profile64_tab[cls].bytes >= size)
        profile64_tab[cls].bytes -= size;
    else
        profile64_tab[cls].bytes = 0;
}

int profile64_get(int cls, u64 *allocs, u64 *bytes, u64 *peak) {
    if (cls < 0 || cls >= PROFILE64_CLASSES) return -1;
    if (allocs) *allocs = profile64_tab[cls].allocs;
    if (bytes) *bytes = profile64_tab[cls].bytes;
    if (peak) *peak = profile64_tab[cls].peak;
    return 0;
}
