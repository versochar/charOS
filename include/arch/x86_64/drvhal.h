#ifndef DRVHAL_H
#define DRVHAL_H

#include "arch/x86_64/longmode.h"

int drvhal64_init(void);
int drvhal64_register(int type, u64 *out_hdl);
int drvhal64_unregister(u64 hdl);
int drvhal64_ioctl(u64 hdl, u64 cmd, u64 arg);
int drvhal64_state(u64 hdl, int *out_state);

#endif
