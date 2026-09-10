#ifndef BLK_H
#define BLK_H

#include "arch/x86_64/longmode.h"

int blk64_init(void);
int blk64_create(u64 nsectors, u64 *out_dev);
int blk64_destroy(u64 dev);
int blk64_read(u64 dev, u64 lba, u64 *out_val);
int blk64_write(u64 dev, u64 lba, u64 val);
int blk64_size(u64 dev, u64 *out_nsectors);

#endif
