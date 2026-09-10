# 11B - Block Device API Spesifikasyonu

## API
```c
int blk64_init(void);
int blk64_create(u64 nsectors, u64 *out_dev);
int blk64_destroy(u64 dev);
int blk64_read(u64 dev, u64 lba, u64 *out_val);
int blk64_write(u64 dev, u64 lba, u64 val);
int blk64_size(u64 dev, u64 *out_nsectors);
```

## Sözleşme
- `create`: nsectors 1-128 dışında -1, NULL out -1, tablo dolu -1.
- `destroy`: bilinmeyen dev -1.
- `read/write`: bilinmeyen dev -1, lba >= nsectors -1, read NULL out -1.

## Uyumluluk
Mevcut blok kodu değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
11C implementasyon başlatma.
