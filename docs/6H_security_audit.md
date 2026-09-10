# 6H - Scheduling Güvenlik Denetimi

## Denetim
- Öncelik tersine çevirme: modelde mutex yok; gerçek RT'de priority-inheritance gerekir (not).
- Açlık: NORMAL sınıf RT varken seçilmez (tasarım gereği); test ile doğrulandı.
- Sahte PID: bilinmeyen PID tüm mutasyonlarda reddedilir.

## Testler
- Duplicate add, unknown remove, aralık dışı prio reddedildi.

## Sonuç
PASSED.
