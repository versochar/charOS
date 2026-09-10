# 9H - Drivers & HAL Güvenlik Denetimi

## Denetim
- Sahte handle: monoton id + active slot kontrolü var.
- Tip karışması: tip kaydı state ile birlikte tutulur.
- Use-after-unregister: slot sıfırlanır, sonrası -1.

## Testler
- Geçersiz tip, bilinmeyen handle, destroy-sonrası ioctl reddedildi.

## Sonuç
PASSED.
