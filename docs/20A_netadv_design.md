# 20A - Advanced Networking Tasarım

## Amaç
charOS için statik yönlendirme tablosu davranış modeli: prefix/mask/gw, first-match lookup.

## Kararlar
- **Tablo**: 16 giriş, monoton route-id. Her giriş prefix/mask/gw.
- **Lookup**: (addr & mask) == prefix eşleşmesi, first-match kazanır. Eşleşme yoksa -1.
- **Yaşam döngüsü**: del slotu FREE yapar, sonrası -1.
- **Güvenlik**: NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/netadv64.c` (host-test davranış modeli)
- `include/arch/x86_64/netadv.h`
- Gerçek FIB (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
20B API spesifikasyonu.
