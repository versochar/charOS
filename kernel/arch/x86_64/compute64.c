/* 47H: compute shaders — gonderim + grup hesabi + paylasim denetimi. */
#include "arch/x86_64/longmode.h"

#define COMPUTE64_MAX_LOCAL 1024
#define COMPUTE64_MAX_SHARED (48 * 1024)

static u64 compute64_done = 0;
static u64 compute64_shared = COMPUTE64_MAX_SHARED;

u64 compute64_groups(u64 total, u64 local) {
    if (!local || local > COMPUTE64_MAX_LOCAL) return 0;
    if (!total) return 0;
    return (total + local - 1) / local;
}

int compute64_dispatch(u64 gx, u64 gy, u64 gz, u64 local) {
    u64 groups;
    if (!gx || !gy || !gz) return -1;
    if (!local || local > COMPUTE64_MAX_LOCAL) return -2;
    if (gx > 65535 || gy > 65535 || gz > 65535) return -3;
    groups = gx * gy * gz;
    compute64_done += groups;
    return 0;
}

int compute64_shared_ok(u64 bytes) {
    return bytes <= compute64_shared ? 1 : 0;
}

u64 compute64_completed(void) { return compute64_done; }
