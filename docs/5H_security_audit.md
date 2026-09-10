# 5H - Process Güvenlik Denetimi

## Denetim
- PID tahmin/spoof: monoton PID + state kontrolü var.
- ZOMBIE bekletme (leak): wait ile FREE zorunlu, testli.
- Slot taşması: dolu tabloda spawn -1 döner.

## Testler
- Bilinmeyen PID, double-exit, READY-wait reddedildi.

## Sonuç
PASSED.
