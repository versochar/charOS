# 19B - Network Security API Spesifikasyonu

## API
```c
int netsec64_init(void);
int netsec64_add(u64 addr, u64 port, int action, u64 *out_rule);
int netsec64_del(u64 rule);
int netsec64_check(u64 addr, u64 port);
```

## Sözleşme
- `add`: action 0-1 dışında -1, NULL out -1, tablo dolu -1.
- `del`: bilinmeyen rule -1.
- `check`: allow eşleşmede 0, deny eşleşmede veya eşleşme yoksa -1.

## Sonraki Adım
19C implementasyon başlatma.
