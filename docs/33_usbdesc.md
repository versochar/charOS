# 33 — USB Tanımlayıcı Sertleştirme (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (33.1–33.10).
Tespit: yapılandırma tanımlayıcı yürüyücü `enum_device` içine gömülüydü;
düşman cihaz girdilerine karşı kilitli test yoktu.

## 33.1 Tasarım
- Yürüyücü saf fonksiyona çıkarılır, BadUSB modeli testlerle kilitlenir.
- Kurallar: L<2 veya pencere-dışı uzunlukta dur (L=0 takılmaz),
  total çağranda budanır, tüm çıkışlar NULL-denetimli.

## 33.2 API (`include/drivers/usbdesc.h`)
- `usbdesc_find_hid_interrupt` + `usbdesc_selftest`; yalnızca `stdint.h`.

## 33.3 Mantık (`kernel/drivers/usbdesc.c`)
- `usbhid.c` içindeki döngüyle birebir mantık + sözleşmeler + öztest
  (geçerli kbd + L=0 düşman girdisi + NULL).
- Aynı dosya çekirdekte ve host'ta derlenir.

## 33.4 Entegrasyon
- `usbhid.c` `enum_device` satır-içi döngü yerine çağrı kullanır
  (davranış aynı: tip/mps/interval atamaları korunur).
- `kernel.c` `[33]` bloğu: `usbdesc` öztest kaydı + sonuç basımı.

## 33.5 Host test (`tests/host/test_usbdesc33.c`, `make test-usbdesc33`)
- GERÇEK `kernel/drivers/usbdesc.c` derlenir: TUMU PASS (13 kontrol:
  kbd, fare, L=0, kesik, OUT, sınıf-dışı, parametreler, determinizm).

## 33.6 Derleme
- `KERNEL_SRCS` += `kernel/drivers/usbdesc.c`; `make build/kernel.bin`
  temiz, `usbdesc_*` sembolleri ELF'te.

## 33.7 Kod inceleme
- [x] Refactor öncesi/sonrası mantık birebir (karşılaştırmalı okuma)
- [x] `rd16` hâlâ kullanımda (ölü kod uyarısı yok)
- [x] mps 0x7FF maskesi + 0→8 varsayılanı korunur
- [x] `usbdesc.c` bağımlılığı: `stdint.h` + `verify.h`

## 33.8 Güvenlik denetimi notu
- L=0 girdisi IRQ bağlamında takılma yapmaz (reddedilir, test kilitli).
- Kesik tanımlayıcı okuma-taşması yapmaz (pencere denetimi).
- Kalan: tam HID rapor-tanımlayıcı ayrıştırıcı yok (boot-protokol
  varsayımı sürer — takip işi).

## 33.9 Dokümantasyon
- Bu dosya + header içi sözleşmeler.

## 33.10 Sürüm
- Versiyon: 33.0.0. `make test-usbdesc33` + çekirdek derlemesi yeşil.
- Dosyalar: `include/drivers/usbdesc.h`, `kernel/drivers/usbdesc.c`,
  `tests/host/test_usbdesc33.c`, `docs/33_usbdesc.md`.
