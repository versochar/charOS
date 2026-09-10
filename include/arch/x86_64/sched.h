#ifndef SCHED_H
#define SCHED_H

#include "arch/x86_64/longmode.h"

int sched64_init(void);
int sched64_add(u64 pid, int prio);
int sched64_remove(u64 pid);
int sched64_set_prio(u64 pid, int prio);
int sched64_current(u64 *out_pid);
int sched64_yield(void);
int sched64_tick(void);

#endif
