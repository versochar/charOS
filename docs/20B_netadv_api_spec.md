# 20B - Advanced Networking API Spesifikasyonu

## API
```c
int netadv64_init(void);
int netadv64_add(u64 prefix, u64 mask, u64 gw, u64 *out_route);
int netadv64_del(u64 route);
int netadv64_lookup(u64 addr, u64 *out_gw);
```

## Sözleşme
- `add`: NULL out -1, tablo dolu -1.
- `del`: bilinmeyen route -1.
- `lookup`: eşleşmede 0 + gw, eşleşme yoksa -1, NULL out -1.

## Sonraki Adım
20C implementasyon başlatma.
