#ifndef NETSEC_H
#define NETSEC_H

#include "arch/x86_64/longmode.h"

int netsec64_init(void);
int netsec64_add(u64 addr, u64 port, int action, u64 *out_rule);
int netsec64_del(u64 rule);
int netsec64_check(u64 addr, u64 port);

#endif
