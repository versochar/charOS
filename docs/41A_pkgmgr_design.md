# 41A - Package Manager Design

## Amaç
Paket yoneticisi: kur/kaldir/sorgula/guncelle. charpkg64 (format) ve paketdepo64 (repo) uzerine katman.

## Kararlar
- Tablo tabanli kayit: name + version + installed.
- Ayni paketi daha eski surumle kurma reddedilir; yeni surum eskiyi upgrade eder.
- Dokunulmazlik: paket boyutu infaz surecinde repo dogrular.

## Bilesenler
- `kernel/arch/x86_64/pkgmgr64.c`, `include/arch/x86_64/pkgmgr.h`

## Basari Kriterleri
- `make test-de64` 41A-J PASS.
