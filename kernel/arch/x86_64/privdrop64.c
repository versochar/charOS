/* 38I: privilege drop — tek-yonlu yetki dusurme + dumpable bayragi. */
#include "arch/x86_64/longmode.h"

static u32 privdrop64_uid = 0;
static u32 privdrop64_gid = 0;
static u32 privdrop64_euid = 0;
static u32 privdrop64_egid = 0;
static int privdrop64_dump_flag = 1;
static int privdrop64_dropped = 0;

int privdrop64_set(u32 uid, u32 gid, u32 euid, u32 egid) {
    privdrop64_uid = uid;
    privdrop64_gid = gid;
    privdrop64_euid = euid;
    privdrop64_egid = egid;
    return 0;
}

/* Dusurme tek yonludur: root -> hedef. Zaten root degilse red. */
int privdrop64_drop(u32 uid, u32 gid) {
    if (privdrop64_euid != 0) return -1; /* root degiliz */
    privdrop64_uid = uid;
    privdrop64_gid = gid;
    privdrop64_euid = uid;
    privdrop64_egid = gid;
    privdrop64_dump_flag = 0;
    privdrop64_dropped = 1;
    return 0;
}

int privdrop64_is_root(void) {
    return privdrop64_euid == 0 ? 1 : 0;
}

int privdrop64_dumpable(void) {
    return privdrop64_dropped ? 0 : privdrop64_dump_flag;
}
