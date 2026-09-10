# 49A - PXE & Network Boot Design

## Amaç
Ag uzerinden boot akisi (`nbp64`): DHCP keşfi, deneme siniri, TFTP indirme, calistirma.

## Kararlar
- **State makinesi**: IDLE -> DHCP -> TFTP_DOWNLOAD -> EXEC (veya FAILED).
- `discover` sunucu ayari olmadan -1; `poll` bootfile yoksa 1 döner ve deneme sayar.
- Maksimum 5 deneme sonrasi FAILED (-2).
- `boot` yalniz TFTP_DOWNLOAD durumunda calisir.

## Bilesenler
- `kernel/arch/x86_64/nbp64.c`, `include/arch/x86_64/nbp.h`
- Mevcut `pxe64` (59G wizard) ile cakismaz.

## Basari Kriterleri
- `make test-de64` 49A-J PASS.
