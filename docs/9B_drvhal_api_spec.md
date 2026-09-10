# 9B - Drivers & HAL API Spesifikasyonu

## API
```c
int drvhal64_init(void);
int drvhal64_register(int type, u64 *out_hdl);
int drvhal64_unregister(u64 hdl);
int drvhal64_ioctl(u64 hdl, u64 cmd, u64 arg);
int drvhal64_state(u64 hdl, int *out_state); /* 0=FREE,1=ACTIVE */
```

## Sözleşme
- `register`: tip 0-2 dışında -1, out NULL -1, tablo dolu -1.
- `unregister`: bilinmeyen hdl -1.
- `ioctl`: bilinmeyen hdl -1.
- `state`: bilinmeyen hdl veya NULL out -1.

## Tipler
0 char, 1 block, 2 net.

## Uyumluluk
Mevcut driverlar değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
9C implementasyon başlatma.
