/* 49E: system monitor — delta-bazli CPU%% + bellek/IO sayaclari. */
#include "arch/x86_64/longmode.h"

static u64 sysmon64_cpu = 0;
static u64 sysmon64_idle = 0;
static u64 sysmon64_cpu_prev = 0;
static u64 sysmon64_idle_prev = 0;
static u64 sysmon64_mem_used = 0;
static u64 sysmon64_disk_rd = 0;
static u64 sysmon64_disk_wr = 0;
static u64 sysmon64_net_rx = 0;
static u64 sysmon64_net_tx = 0;

void sysmon64_tick(u64 cpu, u64 idle, u64 mem) {
    sysmon64_cpu_prev = sysmon64_cpu;
    sysmon64_idle_prev = sysmon64_idle;
    sysmon64_cpu = cpu;
    sysmon64_idle = idle;
    sysmon64_mem_used = mem;
}

int sysmon64_cpu_pct(void) {
    u64 dcpu, didle, total;
    if (sysmon64_cpu < sysmon64_cpu_prev) return 0;
    dcpu = sysmon64_cpu - sysmon64_cpu_prev;
    didle = (sysmon64_idle >= sysmon64_idle_prev)
                ? sysmon64_idle - sysmon64_idle_prev
                : 0;
    total = dcpu + didle;
    if (!total) return 0;
    return (int)(dcpu * 100 / total);
}

u64 sysmon64_mem(void) { return sysmon64_mem_used; }

void sysmon64_io(u64 rd, u64 wr, u64 rx, u64 tx) {
    sysmon64_disk_rd += rd;
    sysmon64_disk_wr += wr;
    sysmon64_net_rx += rx;
    sysmon64_net_tx += tx;
}
