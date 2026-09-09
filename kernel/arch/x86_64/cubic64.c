/* 43C: TCP CUBIC — sabit-noktali pencere buyumesi (olcek 1024).
 * W(t) = C*(t-K)^3 + Wmax, K = (Wmax*(1-beta)/C)^(1/3).
 * Tamsayi kup-kok ikili arama ile (skeleton olcegi).
 */
#include "arch/x86_64/longmode.h"

#define CUBIC64_SCALE 1024
#define CUBIC64_C 410        /* 0.4 * olcek */
#define CUBIC64_BETA 717     /* 0.7 * olcek */

static u64 cubic64_cbrt(u64 x) {
    u64 lo = 0, hi = 1ULL << 21; /* (2^21)^3 = 2^63 */
    if (!x) return 0;
    while (lo + 1 < hi) {
        u64 mid = (lo + hi) / 2;
        __uint128_t m3 = (__uint128_t)mid * mid * mid;
        if (m3 <= x)
            lo = mid;
        else
            hi = mid;
    }
    return lo;
}

void cubic64_init(struct cubic64 *c) {
    if (!c) return;
    c->cwnd = 10 * CUBIC64_SCALE;     /* IW10 */
    c->ssthresh = 64 * CUBIC64_SCALE;
    c->wmax = 0;
    c->epoch = 0;
    c->origin = 0;
}

void cubic64_ack(struct cubic64 *c, u64 rtt_us, u64 now_us) {
    /* W(t) = origin + C*(t-K)^3, K = (origin_pkt*(1-beta)/C)^(1/3).
     * Birimler: cwnd/origin olcekli (x1024), t saniye, C_olcekli=410.
     * diff = 410*(t-k)^3 dogrudan cwnd birimindedir (bolme yok). */
    u64 t, k, target, diff, dt, origin_pkt;
    (void)rtt_us;
    if (!c) return;
    if (c->cwnd < c->ssthresh) {
        c->cwnd += CUBIC64_SCALE; /* yavas baslangic: +1 MSS/ACK */
        return;
    }
    if (!c->epoch) {
        c->epoch = now_us;
        if (c->wmax > c->cwnd)
            c->origin = c->wmax;
        else
            c->origin = c->cwnd;
    }
    t = (now_us - c->epoch) / 1000000;
    origin_pkt = c->origin / CUBIC64_SCALE;
    k = cubic64_cbrt(origin_pkt * (CUBIC64_SCALE - CUBIC64_BETA) /
                     CUBIC64_C);
    dt = (t >= k) ? t - k : k - t;
    if (dt > 100000) dt = 100000; /* tasma emniyeti */
    diff = CUBIC64_C * dt * dt * dt;
    if (t >= k)
        target = c->origin + diff;
    else
        target = (c->origin > diff) ? c->origin - diff : CUBIC64_SCALE;
    /* TCP-dostu bolge: hedefe yavas yaklas */
    if (target > c->cwnd)
        c->cwnd += (target - c->cwnd) / 16 + 1;
    else if (target < c->cwnd && c->cwnd > CUBIC64_SCALE)
        c->cwnd -= (c->cwnd - target) / 16 + 1;
}

void cubic64_loss(struct cubic64 *c) {
    if (!c) return;
    c->wmax = c->cwnd;
    c->cwnd = (c->cwnd * CUBIC64_BETA) / CUBIC64_SCALE;
    if (c->cwnd < 2 * CUBIC64_SCALE) c->cwnd = 2 * CUBIC64_SCALE;
    c->ssthresh = c->cwnd;
    c->epoch = 0;
}
