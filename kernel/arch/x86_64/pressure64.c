/* 32J: bellek basinci — seviye + reclaim karari. Saf C (host testi uygun). */
#include "arch/x86_64/longmode.h"

int pressure64_level(void) {
    u64 total = pmm64_total_frames();
    u64 free = pmm64_free_frames();
    u64 used;
    if (!total) return 100;
    if (free >= total) return 0;
    used = total - free;
    return (int)((used * 100) / total);
}

int pressure64_should_reclaim(void) {
    u64 total = pmm64_total_frames();
    u64 free = pmm64_free_frames();
    if (!total) return 1;
    return free * 20 < total; /* <%5 bos */
}
