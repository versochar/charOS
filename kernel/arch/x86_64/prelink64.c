/* 53A: prelink — kutuphane yukleme adresleri + cakisma denetimi. */
#include "arch/x86_64/longmode.h"

static u64 prelink64_align_up(u64 v, u64 a) {
    if (!a) return v;
    return (v + a - 1) & ~(a - 1);
}

int prelink64_assign(const char *libs[], const u64 sizes[], int n, u64 base,
                     u64 *addrs_out) {
    u64 cur;
    int i;
    if (!libs || !sizes || n <= 0 || !addrs_out) return -1;
    if (base & 0xFFFULL) return -2; /* sayfa hizali taban */
    cur = base;
    for (i = 0; i < n; i++) {
        if (!libs[i] || !sizes[i]) return -3;
        cur = prelink64_align_up(cur, 0x200000ULL); /* 2MB hiza */
        addrs_out[i] = cur;
        cur += prelink64_align_up(sizes[i], 0x1000ULL);
        if (cur < base) return -4; /* tasma */
    }
    return 0;
}

/* 0=cakismasiz, >0 ilk cakisan ciftin index+1. */
int prelink64_verify(const u64 *addrs, const u64 *sizes, int n) {
    int i, j;
    if (!addrs || !sizes || n <= 0) return -1;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            u64 a0 = addrs[i], a1 = addrs[i] + sizes[i];
            u64 b0 = addrs[j], b1 = addrs[j] + sizes[j];
            if (a0 < b1 && b0 < a1) return i + 1;
        }
    }
    return 0;
}
