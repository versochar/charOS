# 40B - Update Mechanism API Spec

## API
```c
int update64_init(void);
int update64_stage(const char *name, const char *ver, u32 size_bytes);
int update64_check_compat(void);
int update64_apply(void);
int update64_verify(void);
int update64_rollback(void);
int update64_commit(void);
int update64_state(int *out);
int update64_staged_count(void);
```

## State Makinesi
- OK -> STAGED (stage) -> APPLYING (apply) -> STAGED -> OK (commit/rollback).
- Hata -> FAILED.
