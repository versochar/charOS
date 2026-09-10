# 21B - Security Framework API Spesifikasyonu

## API
```c
int secfw64_init(void);
int secfw64_add(int hook, u64 subj, u64 obj, int action, u64 *out_rule);
int secfw64_del(u64 rule);
int secfw64_check(int hook, u64 subj, u64 obj);
```

## Sözleşme
- `add`: hook 0-3 dışında -1, action 0-1 dışında -1, NULL out -1.
- `del`: bilinmeyen rule -1.
- `check`: allow eşleşmede 0, aksi halde -1.

## Sonraki Adım
21C implementasyon başlatma.
