#ifndef NETADV_H
#define NETADV_H

#include "arch/x86_64/longmode.h"

int netadv64_init(void);
int netadv64_add(u64 prefix, u64 mask, u64 gw, u64 *out_route);
int netadv64_del(u64 route);
int netadv64_lookup(u64 addr, u64 *out_gw);

#endif
