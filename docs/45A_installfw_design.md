# 45A - Installer Framework Design

## Amaç
Kurulum sihirbazi adim makinesi: on-kontrol (preflight), disk secimi, bolumleme, kopyalama, bootloader.

## Kararlar
- **Adimlar**: PREFLIGHT -> DISK -> PARTITION -> COPY -> BOOTLOADER -> DONE.
- Gecersiz adimda islem -2 ile reddedilir (state makinesi).
- Minimum disk 4 GiB, RAM 512 MiB, bolum 32 MiB.
- Kopyalama toplam boyutu astiginda -4 (tasma korumasi).

## Bilesenler
- `kernel/arch/x86_64/installfw64.c`, `include/arch/x86_64/installfw.h`

## Basari Kriterleri
- `make test-de64` 45A-J PASS.
