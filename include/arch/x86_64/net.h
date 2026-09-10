#ifndef NET_H
#define NET_H

#include "arch/x86_64/longmode.h"

int net64_init(void);
int net64_if_add(int type, u64 *out_if);
int net64_if_del(u64 iface);
int net64_send(u64 iface, u64 val);
int net64_recv(u64 iface, u64 *out_val);
int net64_stat(u64 iface, u64 *out_tx, u64 *out_rx);

#endif
