# 11A - Block Devices & Storage Tasarım

## Amaç
charOS için sabit sektörlü sanal blok cihazı: güvenli LBA erişimi, cihaz yaşam döngüsü.

## Kararlar
- **Cihaz modeli**: 8 slot, cihaz başına 128 sektör (u64/sektör). `create(nsectors)` 1-128 aralığında.
- **Erişim**: `read/write` LBA sınır kontrollü; aralık dışı -1.
- **Yaşam döngüsü**: destroy slotu FREE yapar, sonrası -1.
- **Güvenlik**: NULL out, geçersiz dev/LBA reddedilir.

## Bileşenler
- `kernel/arch/x86_64/blk64.c` (host-test davranış modeli)
- `include/arch/x86_64/blk.h`
- Gerçek NVMe/virtio-blk (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
11B API spesifikasyonu.
