# 32 — Termal Regülatör (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (32.1–32.10).
Tespit: sıcaklık sabit 45°C dönüyordu, pstate yalnızca logluyordu.

## 32.1 Tasarım
- 4'lü kayan ortalama (ani sıçrama yutulur), histerezis bandı 60–75,
  kritik eşik 95 (throttle + sayaç), aralık dışı örnek reddedilir.
- Sıcaklık kaynağı enjekte edilir; QEMU'da DTS olmadığı için çekirdek
  yük-tabanlı model tahminci kullanır (sensör değil, belgelendi).

## 32.2 API (`include/drivers/thermal.h`, mevcut dosya genişletildi)
- Eski API korunur (`get_temp/set_pstate/dynamic_idle/selftest`).
- Yeni: `init/set_sensor/tick/last_temp/trips`.

## 32.3 Mantık (`kernel/drivers/thermal.c`, mevcut dosya gerçeklendi)
- Kayan pencere + histerezis + kritik sayacı; sözleşmeler `REQUIRE` ile.
- `dynamic_idle` artık regülatör adımını koşar (eski kör atama gitti).

## 32.4 Entegrasyon
- `timer.c` 1Hz bloğu `thermal_dynamic_idle()` çağırır (1Hz örnekleme).
- `kernel.c` `[32]` bloğu: init + `thermal` öztest kaydı + durum basımı.
- 32.4-fix: sayaç cap'leme halka indeksini donduruyordu (host test
  yakaladı); sayaç serbest ilerler, pencere `min()` ile sınırlanır.
- Include'lar alıntılı forma çevrildi (host derlemesi için; çekirdek etkilenmez).

## 32.5 Host test (`tests/host/test_thermal32.c`, `make test-thermal32`)
- GERÇEK `kernel/drivers/thermal.c` derlenir (donanım çıktısı stub):
  TUMU PASS (7 kontrol).

## 32.6 Derleme
- `thermal.c` zaten `KERNEL_SRCS` içindeydi; `make build/kernel.bin`
  temiz, 10 `thermal_*` sembolü ELF'te.

## 32.7 Kod inceleme
- [x] Histerezis bandı salınımı yutar (test kanıtlı)
- [x] Kritik sayaç her kritik ortalamada artar
- [x] Aralık dışı örnek durumu bozmaz
- [x] Öztest durumu sıfırlar (side-effect yok)
- [x] u32 sayaç sarmalı halka indeksini bozmaz (modulo)

## 32.8 Güvenlik denetimi notu
- Kritik throttle fail-safe yönündedir (en düşük performans).
- Sensör fn çekirdek-dışı yazılamaz (statik gösterici, yalnızca setter).

## 32.9 Dokümantasyon
- Bu dosya + header içi eşik açıklamaları.

## 32.10 Sürüm
- Versiyon: 32.0.0. `make test-thermal32` + çekirdek derlemesi yeşil.
- Dosyalar: `include/drivers/thermal.h`, `kernel/drivers/thermal.c`,
  `tests/host/test_thermal32.c`, `docs/32_thermal.md`.
