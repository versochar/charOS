# 42A - Repository Management Design

## Amaç
Paket deposu (repository) index yonetimi: paket kaydet, surum filtreli bul, karsilastir.

## Kararlar
- **Index tablosu**: name + ver + repo + enabled kayitlari.
- **Coklu surum**: ayni paketin birden fazla surumu indexte durabilir (exact duplicate reddedilir).
- **repo64_find**: ver_req (>=) filtresiyle en yeni surumu dondurur.
- `repo64_vercmp` segmented karsilastirma (1.5 < 2.0).

## Bilesenler
- `kernel/arch/x86_64/repo64.c` (repo index, host-test davranis modeli)
- Prototipler zaten `longmode.h` 41B blogunda tanimliydi; simdi gerceklendi.

## Basari Kriterleri
- `make test-de64` 42A-J PASS.
