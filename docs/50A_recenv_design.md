# 50A - Recovery Environment Design

## Amaç
Kurtarma ortami yoneticisi (`recenv64`): boot hata sayaci, kurtarma menusu, mod secimi, sayac.

## Kararlar
- **State**: NONE -> MENU -> RESOLVED.
- Boot hatasi 3'e kadar sayilir; 3+ den sonra 0 doner (tavan).
- Menu sure asimi otomatik SAFE modu secilip RESOLVED olur.
- `select` yalniz MENU'de; `confirm` MENU -> RESOLVED.
- `enter` RESOLVED sonrasi -1 (tekrar giris engeli).

## Bilesenler
- `kernel/arch/x86_64/recenv64.c`, `include/arch/x86_64/recenv.h`
- Mevcut `recovery64` (59I wizard) ile cakismaz.

## Basari Kriterleri
- `make test-de64` 50A-J PASS.
