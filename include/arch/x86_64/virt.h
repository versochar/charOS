#ifndef VIRT_H
#define VIRT_H

#include "arch/x86_64/longmode.h"

int virt64_init(void);
int virt64_add(int type, u64 *out_dev);
int virt64_del(u64 dev);
int virt64_kick(u64 dev, u64 q);
int virt64_poll(u64 dev, u64 q, u64 *out_pending);
int virt64_ack(u64 dev, u64 q);

#endif
