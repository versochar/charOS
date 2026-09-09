/* 49B: CPU temp — TjMax - dijital okuma, cekirdek tablosu + azami. */
#include "arch/x86_64/longmode.h"

#define CPUTEMP64_MAX_CORES 16

static int cputemp64_temps[CPUTEMP64_MAX_CORES];
static int cputemp64_n = 0;

int cputemp64_update(int core, u32 tjmax, u32 readout) {
    int t;
    if (core < 0 || core >= CPUTEMP64_MAX_CORES) return -1;
    if (readout > tjmax) return -2;
    t = (int)(tjmax - readout);
    cputemp64_temps[core] = t;
    if (core + 1 > cputemp64_n) cputemp64_n = core + 1;
    return 0;
}

int cputemp64_core(int core) {
    if (core < 0 || core >= cputemp64_n) return -1000;
    return cputemp64_temps[core];
}

int cputemp64_max(void) {
    int i, m = -1000;
    for (i = 0; i < cputemp64_n; i++) {
        if (cputemp64_temps[i] > m) m = cputemp64_temps[i];
    }
    return m;
}
