/* 36H: per-CPU arena — CPU basina dergi (magazine) onbellegi, slab64 arkasinda.
 * Siniflar {32,128,512,2048}; dergi dolunca slab'a, bosalinca slab'dan.
 */
#include "arch/x86_64/longmode.h"

#define ARENA64_CLASSES 4
#define ARENA64_MAG 8

static const u64 arena64_sizes[ARENA64_CLASSES] = {32, 128, 512, 2048};

static void *arena64_mag[MAX_CPUS64][ARENA64_CLASSES][ARENA64_MAG];
static int arena64_n[MAX_CPUS64][ARENA64_CLASSES];

static int arena64_cls(u64 size) {
    int i;
    for (i = 0; i < ARENA64_CLASSES; i++)
        if (size <= arena64_sizes[i] && size > 0) return i;
    return -1;
}

void *arena64_alloc(int cpu, u64 size) {
    int c;
    if (cpu < 0 || cpu >= MAX_CPUS64) return 0;
    c = arena64_cls(size);
    if (c < 0) return 0;
    if (arena64_n[cpu][c] > 0)
        return arena64_mag[cpu][c][--arena64_n[cpu][c]];
    return slab64_alloc(arena64_sizes[c]);
}

void arena64_free(int cpu, void *p, u64 size) {
    int c;
    if (!p || cpu < 0 || cpu >= MAX_CPUS64) return;
    c = arena64_cls(size);
    if (c < 0) return;
    if (arena64_n[cpu][c] < ARENA64_MAG) {
        arena64_mag[cpu][c][arena64_n[cpu][c]++] = p;
        return;
    }
    slab64_free(p, arena64_sizes[c]);
}
