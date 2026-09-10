# 55B - Kernel Self Protection API Spec

## API
```c
int kprot64_init(void);
int kprot64_set_lockdown(int level);   /* NONE=0..PGD=3, monotonik */
int kprot64_get_lockdown(void);
int kprot64_set_kptr(int mode);       /* ALL/RESTRICT/ZERO */
int kprot64_mask_ptr(u64 raw, int privileged);
int kprot64_set_dmesg_restrict(int on);
int kprot64_dmesg_allowed(int privileged);
int kprot64_lock_rodata(void);
int kprot64_rodata_write(const void *addr);
int kprot64_set_oops_limit(int limit);
int kprot64_oops_count(void);
int kprot64_on_oops(void);
int kprot64_panic_required(void);
int kprot64_restricted_count(void);
```

## Donus Kurallari
- set_lockdown: gecersiz seviye -1; geri alma -2.
- set_kptr: gecersiz mod -1.
- rodata_write: NULL -1; kilitliyken -2.
- on_oops: artan sayaci dondurur; panic gerekligi oops>=limit.
