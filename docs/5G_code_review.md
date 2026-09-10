# 5G - Process Kod İncelemesi

## Kontrol Listesi
- [x] API 5B sözleşmesine uygun
- [x] Hata kodları tutarlı (0/-1)
- [x] NULL kontrolleri tam
- [x] Slot taşması korunuyor (64 slot, dolunca -1)
- [x] Mevcut çekirdek bozulmadı

## Bulgu
Yok (gerçek scheduler'da preemption kilidi gerekir - not).

## Onay
Onaylandı.

## Sonraki Adım
5H güvenlik denetimi.
