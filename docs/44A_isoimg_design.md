# 44A - Live ISO Build Design

## Amaç
Boot edilebilir canli ISO yerlesimini (ISO9660 + El Torito) modelleyen insa yoneticisi.

## Kararlar
- **Nasil calisir**: Dosyalar indexe eklenir, boot dosyasi secilir, layout hesaplanir (16 sektor system area + PVD sonrasi dosya bolgeleri).
- **Sektor**: 2048 bayt. Dosya boyutu ceil(size/2048) sektore yuvarlanir.
- **Boot modlari**: El Torito floppy veya no-emulation; boot dosyasi eklenmedigi surece layout reddedilir.
- Ayni isim iki kez eklenemez; bos/asi boyut reddedilir.

## Bilesenler
- `kernel/arch/x86_64/isoimg64.c`, `include/arch/x86_64/isoimg.h`

## Basari Kriterleri
- `make test-de64` 44A-J PASS.
