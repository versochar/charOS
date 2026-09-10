#ifndef SYNC_H
#define SYNC_H

#include "arch/x86_64/longmode.h"

int sync64_init(void);
int spin64_create(u64 *out_id);
int spin64_trylock(u64 id);
int spin64_unlock(u64 id);
int spin64_destroy(u64 id);
int mutex64_create(u64 *out_id);
int mutex64_lock(u64 id, u64 owner);
int mutex64_unlock(u64 id, u64 owner);
int mutex64_destroy(u64 id);

#endif
