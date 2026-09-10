# 7H - IPC Güvenlik Denetimi

## Denetim
- Kanal spoof: monoton chan-id + valid slot kontrolü var.
- Mesaj sızıntısı: destroy sonrası slot sıfırlanır.
- DoS: dolu mailboxta send -1 döner, bloklama yok.

## Testler
- Bilinmeyen chan, dolu/boş durumları reddedildi.

## Sonuç
PASSED.
