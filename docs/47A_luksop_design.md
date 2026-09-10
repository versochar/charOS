# 47A - LUKS Integration Design

## Amaç
LUKS cilt yonetimi katmani (`luksop64`): formatla, anahtar slotu ekle/dogrula, kilitle/ac.

## Kararlar
- **Cilt**: dev + magic + cipher + key_bits + 8 anahtar slotu + state + CRC.
- **KDF**: kdf_iter (>=1000) + key_material ozeti; slot crc si slot_key.
- **Unlock**: slot dogrulamasi gecerse state UNLOCKED; zaten acikken -3.
- Acikken reformat reddedilir (-2); kapaliyken reformat serbest.
- Header CRC: magic+key_bits+state+slotlar; slot eklenince degisir.

## Bilesenler
- `kernel/arch/x86_64/luksop64.c`, `include/arch/x86_64/luksop.h`
- Mevcut `luks64` (50G) ile cakismaz.

## Basari Kriterleri
- `make test-de64` 47A-J PASS.
