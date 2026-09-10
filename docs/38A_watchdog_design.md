# 38A - Watchdog Design

## Amaç
charOS kernel için watchdog (WDT) tasarımı: pet/feed/reset sayacı, timeout koruma.

## Kararlar
- **Model**: 8-bit timeout (tick), pet/feed sayacı.
- **Reset**: pet/feed çağrılmadığında watchdog sayacı artar; timeout aşıldığında reset.
- **Durum**: `WDT_OK`, `WDT_TIMEOUT`, `WDT_RESET`.

## Bileşenler
- `kernel/arch/x86_64/watchdog64.c` (host-test davranış modeli)
- `include/arch/x86_64/watchdog.h`

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` yeşil.
