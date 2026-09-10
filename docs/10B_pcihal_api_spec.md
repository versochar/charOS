# 10B - PCI HAL API Spesifikasyonu

## API
```c
int pcihal64_init(void);
int pcihal64_scan(u64 *out_count);
int pcihal64_info(u64 idx, u64 *out_vendor, u64 *out_device);
int pcihal64_enable(u64 idx);
int pcihal64_read(u64 idx, u64 offset, u64 *out_val);
```

## Sözleşme
- `scan`: NULL out -1.
- `info`: idx >= count veya NULL out -1.
- `enable`: idx geçersiz -1, zaten active -1.
- `read`: idx geçersiz/pasif -1, offset >255 -1, NULL out -1.

## Uyumluluk
Mevcut PCI kodu değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
10C implementasyon başlatma.
