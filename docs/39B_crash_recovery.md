# 39B - Crash Dump API Spec

## API
```c
int crash64_init(void);
int crash64_begin(u8 reason);
int crash64_write_regs(const u32 *regs, int nregs);
int crash64_save_stack(const void *sp, u32 bytes);
int crash64_finalize(void);           // seq dondurur
int crash64_find(int seq, struct crash64_hdr *out);
int crash64_read_payload(int seq, void *out, u32 max);
int crash64_reboot_after_dump(int seq);
int crash64_count(void);
```
