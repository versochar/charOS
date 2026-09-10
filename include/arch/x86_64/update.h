#ifndef UPDATE64_H
#define UPDATE64_H
#include "arch/x86_64/longmode.h"

#define UPDATE64_MAX_PKGS 64
#define UPDATE64_VER_MAX   48

enum update64_state {
    UPDATE_OK = 0,
    UPDATE_STAGED = 1,
    UPDATE_APPLYING = 2,
    UPDATE_FAILED = 3,
};

int update64_init(void);
int update64_stage(const char *name, const char *ver, u32 size_bytes);
int update64_check_compat(void);
int update64_apply(void);
int update64_verify(void);
int update64_rollback(void);
int update64_commit(void);
int update64_state(int *out);
int update64_staged_count(void);

#endif