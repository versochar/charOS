#include "arch/x86_64/dyna.h"
#include <string.h>
#include <stdio.h>

struct probe {
    u32 addr;
    int gran;
    int used;
};

static struct probe probes[DYNA64_MAX_PROBES];
static int probe_cnt = 0;
static int running = 0;
static int evt_ctr[6];
static int steps = 0;
static int cfg[64];
static int initialized = 0;
static u32 fake_pc;
static int fake_event;

int dyna64_init(void) {
    memset(probes, 0, sizeof(probes));
    probe_cnt = 0;
    memset(evt_ctr, 0, sizeof(evt_ctr));
    running = 0;
    steps = 0;
    memset(cfg, 0, sizeof(cfg));
    initialized = 1;
    fake_pc = 0x1000;
    fake_event = DYNA64_E_INSTR;
    return 0;
}

int dyna64_probe_add(u32 addr, int granularity) {
    if (!initialized) return -1;
    if (probe_cnt >= DYNA64_MAX_PROBES) return -2;
    if (granularity <= 0) return -3;
    probes[probe_cnt].addr = addr;
    probes[probe_cnt].gran = granularity;
    probes[probe_cnt].used = 1;
    probe_cnt++;
    return 0;
}

int dyna64_probe_remove(u32 addr) {
    int i, j;
    for (i = 0; i < probe_cnt; i++) {
        if (probes[i].used && probes[i].addr == addr) {
            for (j = i; j < probe_cnt - 1; j++) probes[j] = probes[j + 1];
            probe_cnt--;
            return 0;
        }
    }
    return -1;
}

int dyna64_start(void) {
    if (!initialized) return -1;
    running = 1;
    return 0;
}

int dyna64_stop(void) {
    if (!initialized) return -1;
    running = 0;
    return 0;
}

int dyna64_step(u32 *out_pc, int *out_event) {
    if (!initialized) return -1;
    if (!running) return -2;
    if (!out_pc || !out_event) return -3;
    fake_pc += 4;
    steps++;
    fake_event = (fake_event + 1) % 6;
    *out_pc = fake_pc;
    *out_event = fake_event;
    evt_ctr[fake_event]++;
    return 0;
}

int dyna64_event_count(int ev) {
    if ((int)ev < 0 || (int)ev > 5) return -1;
    return evt_ctr[ev];
}

int dyna64_log_dump(char *buf, int max) {
    int i;
    if (!buf || max <= 0) return -1;
    if (max < 32) return -2;
    snprintf(buf, (size_t)max, "probes=%d steps=%d", probe_cnt, steps);
    for (i = 0; i < 6; i++) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), " e%d=%d", i, evt_ctr[i]);
        strncat(buf, tmp, (size_t)(max - strlen(buf) - 1));
    }
    return 0;
}

int dyna64_trace_flush(void) {
    memset(evt_ctr, 0, sizeof(evt_ctr));
    steps = 0;
    return 0;
}

int dyna64_stack_snapshot(u32 stack[DYNA64_MAX_STACK], int *depth) {
    int i;
    if (!stack || !depth) return -1;
    for (i = 0; i < DYNA64_MAX_STACK; i++) stack[i] = 0x10000000u + (u32)(i * 4);
    *depth = DYNA64_MAX_STACK;
    return 0;
}

int dyna64_config_set(int key, int val) {
    if (key < 0 || key >= 64) return -1;
    cfg[key] = val;
    return 0;
}

int dyna64_config_get(int key, int *out) {
    if (!out) return -1;
    if (key < 0 || key >= 64) return -2;
    *out = cfg[key];
    return 0;
}