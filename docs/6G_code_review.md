# 6G - Scheduling Kod İncelemesi

## Kontrol Listesi
- [x] API 6B sözleşmesine uygun
- [x] Hata kodları tutarlı (0/-1)
- [x] NULL/aralık kontrolleri tam
- [x] Tablo taşması korunuyor (64 görev)
- [x] Mevcut çekirdek bozulmadı

## Bulgu
Yok (gerçek SMP'de runqueue başına kilit gerekir - not).

## Onay
Onaylandı.

## Sonraki Adım
6H güvenlik denetimi.
