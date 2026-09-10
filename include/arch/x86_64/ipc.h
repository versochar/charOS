#ifndef IPC_H
#define IPC_H

#include "arch/x86_64/longmode.h"

int ipc64_init(void);
int ipc64_create(u64 *out_chan);
int ipc64_destroy(u64 chan);
int ipc64_send(u64 chan, u64 msg);
int ipc64_recv(u64 chan, u64 *out_msg);

#endif
