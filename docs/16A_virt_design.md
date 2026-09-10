# 16A - Virtio & Virtualization Tasarım

## Amaç
charOS için virtqueue davranış modeli: kick/poll/ack sayaçları, cihaz yaşam döngüsü.

## Kararlar
- **Cihaz modeli**: 8 slot, monoton dev-id. Tip 0=net,1=blk,2=console.
- **Kuyruk**: cihaz başına 4 kuyruk, her biri pending sayacı.
- **Semantik**: `kick` artırır, `poll` okur, `ack` varsa bir azaltır.
- **Güvenlik**: geçersiz dev/q, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/virt64.c` (host-test davranış modeli)
- `include/arch/x86_64/virt.h`
- Gerçek virtio MMIO (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
16B API spesifikasyonu.
