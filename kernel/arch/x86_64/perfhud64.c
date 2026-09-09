/* 57D: performans HUD — CPU/RAM ölçüm + ekran üstü. */
#include "arch/x86_64/longmode.h"

struct perf_state {
    int active;
    u64 cpu_pct;
    u64 ram_used;
    u64 ram_total;
    int overlay;
    int x;
    int y;
    int refresh_ms;
};

static struct perf_state perf = {0};

int perf64_init(void) {
    if (perf.active) return -1;
    perf.active = 1;
    perf.cpu_pct = 0;
    perf.ram_used = 0;
    perf.ram_total = 0;
    perf.overlay = 1;
    perf.x = 10;
    perf.y = 10;
    perf.refresh_ms = 1000;
    return 0;
}

int perf64_shutdown(void) {
    if (!perf.active) return -1;
    perf.active = 0;
    return 0;
}

int perf64_update_cpu(u64 pct) {
    if (!perf.active) return -1;
    if (pct > 100) return -2;
    perf.cpu_pct = pct;
    return 0;
}

int perf64_update_ram(u64 used, u64 total) {
    if (!perf.active) return -1;
    if (total == 0) return -2;
    perf.ram_used = used;
    perf.ram_total = total;
    return 0;
}

int perf64_get_cpu(u64 *out) {
    if (!perf.active || !out) return -1;
    *out = perf.cpu_pct;
    return 0;
}

int perf64_get_ram_used(u64 *out) {
    if (!perf.active || !out) return -1;
    *out = perf.ram_used;
    return 0;
}

int perf64_get_ram_total(u64 *out) {
    if (!perf.active || !out) return -1;
    *out = perf.ram_total;
    return 0;
}

int perf64_overlay(int on) {
    if (!perf.active) return -1;
    perf.overlay = on ? 1 : 0;
    return 0;
}

int perf64_pos(int x, int y) {
    if (!perf.active) return -1;
    if (x < 0 || y < 0) return -2;
    perf.x = x;
    perf.y = y;
    return 0;
}

int perf64_get_pos(int *x, int *y) {
    if (!perf.active || !x || !y) return -1;
    *x = perf.x;
    *y = perf.y;
    return 0;
}

int perf64_refresh(int ms) {
    if (!perf.active) return -1;
    if (ms <= 0) return -2;
    perf.refresh_ms = ms;
    return 0;
}

int perf64_get_refresh(int *out) {
    if (!perf.active || !out) return -1;
    *out = perf.refresh_ms;
    return 0;
}
