# 40A - Update Mechanism Design

## Amaç
Kernel update mekanizmasi: paketleri stage et, uyumlulugu kontrol et, uygula/geri al/onayla.

## Kararlar
- **Stage/Apply/Commit**: Surumler once hazirlanir, uygulanir, dogrulanir, onaylanir.
- **Rollback**: Apply sonrasi state STAGED iken geri alinabilir.
- **Boyut siniri**: Paket basina 1 GiB (0x40000000).

## Bilesenler
- `kernel/arch/x86_64/update64.c` (host-test davranis modeli)
- `include/arch/x86_64/update.h`

## Basari Kriterleri
- `make test-de64` 40A-J PASS.
