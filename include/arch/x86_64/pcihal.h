#ifndef PCIHAL_H
#define PCIHAL_H

#include "arch/x86_64/longmode.h"

int pcihal64_init(void);
int pcihal64_scan(u64 *out_count);
int pcihal64_info(u64 idx, u64 *out_vendor, u64 *out_device);
int pcihal64_enable(u64 idx);
int pcihal64_read(u64 idx, u64 offset, u64 *out_val);

#endif
