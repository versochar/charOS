#ifndef SECFW_H
#define SECFW_H

#include "arch/x86_64/longmode.h"

int secfw64_init(void);
int secfw64_add(int hook, u64 subj, u64 obj, int action, u64 *out_rule);
int secfw64_del(u64 rule);
int secfw64_check(int hook, u64 subj, u64 obj);

#endif
