/* 51I: math lib — tamsayi kok/log/us + sabit-nokta trig + eboB/ekoK. */
#include "arch/x86_64/longmode.h"

u64 math64_isqrt(u64 x) {
    u64 lo = 0, hi = (x >= (1ULL << 32)) ? (1ULL << 32) : x + 1;
    if (x < 2) return x;
    while (lo + 1 < hi) {
        u64 mid = lo + (hi - lo) / 2;
        if (mid <= x / mid)
            lo = mid;
        else
            hi = mid;
    }
    return lo;
}

u64 math64_icbrt(u64 x) {
    u64 lo = 0, hi = 1ULL << 22; /* 2^22^3 = 2^66 > 2^64 */
    if (!x) return 0;
    while (lo + 1 < hi) {
        u64 mid = lo + (hi - lo) / 2;
        __uint128_t m3 = (__uint128_t)mid * mid * mid;
        if (m3 <= x)
            lo = mid;
        else
            hi = mid;
    }
    return lo;
}

int math64_ilog2(u64 x) {
    int r = -1;
    if (!x) return -1;
    while (x) {
        x >>= 1;
        r++;
    }
    return r;
}

u64 math64_pow_u64(u64 base, int exp) {
    u64 r = 1;
    if (exp < 0) return 0;
    while (exp) {
        if (exp & 1) r *= base;
        exp >>= 1;
        if (exp) base *= base;
    }
    return r;
}

u64 math64_gcd(u64 a, u64 b) {
    while (b) {
        u64 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/* Q16 sinus (derece): 90'lik simetri + 16'li ceyrek tablo (0..90). */
int math64_sin_q16(int degrees) {
    static const int table[16] = {0,     3425,  6812,  10126,
                                  13327, 16384, 19260, 21928,
                                  24351, 26509, 28377, 29938,
                                  31163, 32051, 32587, 32767};
    int d = degrees % 360, q, idx, v;
    if (d < 0) d += 360;
    q = d / 90;
    idx = (d % 90) * 16 / 90;
    if (idx > 15) idx = 15;
    v = table[idx];
    if (q == 1) v = table[15 - idx];
    if (q == 2) v = -table[idx];
    if (q == 3) v = -table[15 - idx];
    return v * 2; /* x65536 olcegi (32767*2) */
}

int math64_cos_q16(int degrees) {
    return math64_sin_q16(degrees + 90);
}
