# 4H - Advanced Memory Güvenlik Denetimi

## Denetim
- KASAN bypass: poison bitmap sınır kontrolü var, taşma yok.
- Huge-map çakışma: -2 yolu tanımlı, modelde hizalama reddi doğrulandı.
- Integer overflow: order/size çarpımı modelde sabit; gerçek PMM'de order < 32 zorunlu (not).

## Testler
- Geçersiz level, hizasız adres, NULL out reddedildi.

## Sonuç
PASSED (davranış modeli; gerçek sürücüde fuzz + KASAN self-test önerilir).
