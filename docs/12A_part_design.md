# 12A - Partition Manager Tasarım

## Amaç
charOS için GPT-benzeri bölüm tablosu: çakışmasız aralık kaydı, tip etiketi.

## Kararlar
- **Tablo**: 16 giriş, monoton part-id. Her giriş dev/start/len/type.
- **Çakışma**: aynı dev üzerinde [start,start+len) kesişirse -2.
- **Yaşam döngüsü**: del slotu FREE yapar, sonrası -1.
- **Güvenlik**: len==0, NULL out, bilinmeyen id reddedilir.

## Bileşenler
- `kernel/arch/x86_64/part64.c` (host-test davranış modeli)
- `include/arch/x86_64/part.h`
- Gerçek GPT parse (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
12B API spesifikasyonu.
