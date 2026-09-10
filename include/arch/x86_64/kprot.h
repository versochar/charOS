#ifndef KPROT64_H
#define KPROT64_H
#include "arch/x86_64/longmode.h"

/* Kernel Self Protection politikasi */
#define KPROT64_LOCKDOWN_NONE           0
#define KPROT64_LOCKDOWN_CONFIDENTIAL   1
#define KPROT64_LOCKDOWN_INTEGRITY      2
#define KPROT64_LOCKDOWN_PGD            3

#define KPROT64_KPTR_ALL       0x0u   /* herkes gorebilir */
#define KPROT64_KPTR_RESTRICT  0x1u   /* init/ayri calik disinda gizle */
#define KPROT64_KPTR_ZERO      0x2u   /* tamamen sifirla */

int kprot64_init(void);
int kprot64_set_lockdown(int level);
int kprot64_get_lockdown(void);
int kprot64_set_kptr(int mode);
int kprot64_kptr(int privileged);
u64  kprot64_mask_ptr(u64 raw, int privileged);
int kprot64_set_dmesg_restrict(int on);
int kprot64_dmesg_allowed(int privileged);
int kprot64_lock_rodata(void);
int kprot64_rodata_write(const void *addr);
int kprot64_set_oops_limit(int limit);
int kprot64_oops_count(void);
int kprot64_on_oops(void);
int kprot64_panic_required(void);
int kprot64_restricted_count(void);

#endif