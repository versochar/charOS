# 12B - Partition API Spesifikasyonu

## API
```c
int part64_init(void);
int part64_add(u64 dev, u64 start, u64 len, int type, u64 *out_part);
int part64_del(u64 part);
int part64_info(u64 part, u64 *out_dev, u64 *out_start, u64 *out_len);
```

## Sözleşme
- `add`: len==0 -1, NULL out -1, çakışma -2, tablo dolu -1.
- `del`: bilinmeyen part -1.
- `info`: bilinmeyen part veya NULL out -1.

## Tipler
0 unused, 1 EFI, 2 data, 3 swap.

## Sonraki Adım
12C implementasyon başlatma.
