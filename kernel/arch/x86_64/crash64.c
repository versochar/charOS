#include "arch/x86_64/crash.h"
#include <string.h>

#define DUMP_SLOT 4096u
static int cr_active = 0;
static u16 cr_seq = 0;
static u8 cr_buf[DUMP_SLOT];
static u32 cr_len = 0;
static u8 cr_reason = 0;
static struct crash64_hdr cr_tab[CRASH64_MAX_DUMPS];
static u8 cr_tab_used[CRASH64_MAX_DUMPS];

static u32 cr_crc32(const void *data, u32 n) {
    const u8 *p = (const u8 *)data;
    u32 crc = 0xFFFFFFFFu;
    u32 i;
    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (u32)-(crc & 1u));
    }
    return crc ^ 0xFFFFFFFFu;
}

int crash64_init(void) {
    cr_active = 0;
    cr_len = 0;
    cr_reason = 0;
    cr_seq = 0;
    memset(cr_tab_used, 0, sizeof(cr_tab_used));
    return 0;
}

int crash64_begin(u8 reason) {
    if (cr_active) return -1;
    if (reason > 4) return -1;
    cr_active = 1;
    cr_reason = reason;
    cr_len = 0;
    return 0;
}

int crash64_write_regs(const u32 *regs, int nregs) {
    if (!cr_active) return -1;
    if (!regs || nregs <= 0 || nregs > 32) return -1;
    if (cr_len + (u32)nregs * 4 > DUMP_SLOT - 64) return -2;
    memcpy(cr_buf + cr_len, regs, (u32)nregs * 4);
    cr_len += (u32)nregs * 4;
    return 0;
}

int crash64_save_stack(const void *sp, u32 bytes) {
    if (!cr_active) return -1;
    if (!sp || bytes == 0 || bytes > 2048) return -1;
    if (cr_len + bytes > DUMP_SLOT - 64) return -2;
    memcpy(cr_buf + cr_len, sp, bytes);
    cr_len += bytes;
    return 0;
}

int crash64_finalize(void) {
    struct crash64_hdr h;
    int slot = -1;
    int i;
    if (!cr_active) return -1;
    for (i = 0; i < CRASH64_MAX_DUMPS; i++) {
        if (!cr_tab_used[i]) { slot = i; break; }
    }
    if (slot < 0) return -2;
    cr_seq++;
    h.magic = CRASH64_DUMP_MAGIC;
    h.reason = cr_reason;
    h.valid = 1;
    h.seq = cr_seq;
    h.crc = cr_crc32(cr_buf, cr_len);
    cr_tab[slot] = h;
    cr_tab_used[slot] = 1;
    cr_active = 0;
    return (int)cr_seq;
}

int crash64_find(int seq, struct crash64_hdr *out_hdr) {
    int i;
    if (!out_hdr) return -1;
    for (i = 0; i < CRASH64_MAX_DUMPS; i++) {
        if (cr_tab_used[i] && cr_tab[i].seq == (u16)seq) {
            *out_hdr = cr_tab[i];
            return i;
        }
    }
    return -1;
}

int crash64_read_payload(int seq, void *out, u32 maxbytes) {
    int idx = crash64_find(seq, NULL);
    if (idx < 0) return -1;
    if (!out) return -1;
    if (maxbytes < cr_len) return -2;
    memcpy(out, cr_buf, cr_len);
    return (int)cr_len;
}

int crash64_reboot_after_dump(int seq) {
    int idx = crash64_find(seq, NULL);
    if (idx < 0) return -1;
    cr_tab_used[idx] = 0;
    return 0;
}

int crash64_count(void) {
    int i, c = 0;
    for (i = 0; i < CRASH64_MAX_DUMPS; i++)
        if (cr_tab_used[i]) c++;
    return c;
}