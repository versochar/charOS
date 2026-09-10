# 43B - Secure Update Signing API Spec

## API
```c
int secupd64_init(void);
int secupd64_set_policy(int policy);
int secupd64_add_key(int id, u64 secret);
u64 secupd64_sign(const char *name, const char *ver, u64 payload_hash, int key_id);
int secupd64_stage(const char *name, const char *ver, u64 payload_hash, u64 sig, int key_id);
int secupd64_verify(const char *name, const char *ver);
int secupd64_apply_ok(const char *name, const char *ver);
int secupd64_policy(int *out);
int secupd64_staged_count(void);
```

## Donus Kurallari
- stage: NULL -1; tablo dolu -2; anahtar yok -3; imza eslesmez -4.
- verify: kayit yok -4; anahtar kaybolmus -2; imza bozulmus -3.
