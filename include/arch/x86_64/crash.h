#ifndef CRASH64_H
#define CRASH64_H
#include "arch/x86_64/longmode.h"

#define CRASH64_MAX_DUMPS 16
#define CRASH64_DUMP_MAGIC 0x43524153u /* "CRAS" */

struct crash64_hdr {
    u32 magic;
    u8  reason;                /* 0 none, 1 PF, 2 GP, 3 INT, 4 oom */
    u8  valid;
    u16 seq;                   /* dump counter */
    u32 crc;                   /* checksum of payload */
};

int crash64_init(void);
int crash64_begin(u8 reason);
int crash64_write_regs(const u32 *regs, int nregs);
int crash64_save_stack(const void *sp, u32 bytes);
int crash64_finalize(void);
int crash64_find(int seq, struct crash64_hdr *out_hdr);
int crash64_read_payload(int seq, void *out, u32 maxbytes);
int crash64_reboot_after_dump(int seq);
int crash64_count(void);

#endif