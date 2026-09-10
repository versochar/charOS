#ifndef PART_H
#define PART_H

#include "arch/x86_64/longmode.h"

int part64_init(void);
int part64_add(u64 dev, u64 start, u64 len, int type, u64 *out_part);
int part64_del(u64 part);
int part64_info(u64 part, u64 *out_dev, u64 *out_start, u64 *out_len);

#endif
