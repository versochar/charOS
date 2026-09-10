# 8H - Synchronization Güvenlik Denetimi

## Denetim
- Yetkisiz unlock: mutex owner kontrolü var, yanlış owner -1.
- Kilit sızıntısı: destroy kilitli de olsa slotu temizler (testli).
- DoS: spin busy'de -1 döner, sonsuz bekleme yok (v1 non-blocking).

## Testler
- Double-lock, kilitsiz unlock, yanlış-owner reddedildi.

## Sonuç
PASSED.
