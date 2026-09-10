#ifndef JRNL_H
#define JRNL_H

#include "arch/x86_64/longmode.h"

int jrnl64_init(void);
int jrnl64_begin(u64 *out_tx);
int jrnl64_append(u64 tx, u64 val);
int jrnl64_commit(u64 tx);
int jrnl64_abort(u64 tx);
int jrnl64_replay(u64 *out_count);

#endif
