# 51B - Container Runtime API Spec

## API
```c
int ctr64_init(void);
int ctr64_create(const char *name, const char *image, u64 mem_limit_bytes, int *out_id);
int ctr64_start(int id);
int ctr64_pause(int id);
int ctr64_resume(int id);
int ctr64_stop(int id);
int ctr64_state(int id, int *out);
int ctr64_count(void);
int ctr64_pid(int id, u64 *out_pid);
```

## Donus Kurallari
- create: NULL -1; mem=0 -2; ayni isim -3; dolu -4.
- pause/resume/stop: bilinmeyen id -1; gecersiz durum -2.
