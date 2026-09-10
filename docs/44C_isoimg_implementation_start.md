# 44C - Live ISO Build Implementation Start

## Yapilanlar
- `isoimg64.c`: dosya tablosu, sektor hesaplama, boot secimi.
- `isoimg.h` + `longmode.h` prototipleri.

## Test
44C1-44C5 PASS (4096B -> 2 sektor, 16+PVD -> 18).
