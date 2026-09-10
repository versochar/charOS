# 10A - PCI HAL Tasarım

## Amaç
charOS için sahte- deterministik PCI tarama + enable/read davranış modeli: 4 sabit cihaz, güvenli index erişimi.

## Kararlar
- **Cihaz tablosu**: 4 giriş (vendor/device sabit). `scan` sayıyı döner.
- **Enable**: index geçerliyse active=1, tekrar enable -1.
- **Read**: offset 0-255, active cihazda 0; pasif/geçersizde -1.
- **Güvenlik**: aralık dışı index/offset, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/pcihal64.c` (host-test davranış modeli)
- `include/arch/x86_64/pcihal.h`
- Gerçek PCI config-space access (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
10B API spesifikasyonu.
