# 9A - Drivers & HAL Tasarım

## Amaç
charOS için tip-etiketli sürücü kaydı + ioctl arayüzü: blok/char/net tipleri, güvenli register/unregister.

## Kararlar
- **Kayıt modeli**: 64 slot, monoton handle. Tip alanı 0=char,1=block,2=net; aralık dışı tip reddedilir.
- **Ioctl**: `cmd` + `arg` iletir, dönüş 0; bilinmeyen handle -1.
- **Yaşam döngüsü**: unregister slotu FREE yapar, sonrası kullanım -1.
- **Güvenlik**: NULL out, geçersiz tip/handle reddedilir.

## Bileşenler
- `kernel/arch/x86_64/drvhal64.c` (host-test davranış modeli)
- `include/arch/x86_64/drvhal.h`
- Gerçek MMIO/DMA sürücüler (plan) rezerve; mevcut driverlar bozulmaz.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
9B API spesifikasyonu.
