#ifndef PROC_H
#define PROC_H

#include "arch/x86_64/longmode.h"

int proc64_init(void);
int proc64_spawn(u64 *out_pid);
int proc64_exit(u64 pid, int code);
int proc64_wait(u64 pid, int *out_code);
int proc64_state(u64 pid, int *out_state);

#endif
