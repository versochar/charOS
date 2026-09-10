# 48A - Bootloader Integration Design

## Amaç
Bootloader yapilandirma katmani: menu girdileri, varsayilan girdi, zaman asimi, kernel/initrd dogrulama.

## Kararlar
- **Girdi**: label + kernel + initrd + options.
- kernel path `/boot/` ile baslamali ve `.bin`/`.elf` ile bitmeli; `..` (traversal) yasak.
- initrd `/boot/` altinda olmali.
- Varsayilan girdi indeksi; zaman asimi 0..300 sn.

## Bilesenler
- `kernel/arch/x86_64/blcfg64.c`, `include/arch/x86_64/blcfg.h`
- Mevcut `bootloader64` (59E wizard) ile cakismaz.

## Basari Kriterleri
- `make test-de64` 48A-J PASS.
