# 35 — Kırpma Çekirdeği (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (35.1–35.10).
Tespit: çizim kodu int toplamada teorik taşma UB'si taşıyordu
(`x+w` INT_MAX ile tanımsız davranış).

## 35.1 Tasarım
- Kırpma matematiği saf fonksiyona çıkarılır, ara hesap 64-bit.
- Sözleşme: görünürse 1 + doldurulmuş çıktı, yoksa 0.

## 35.2 API (`include/drivers/raster.h`)
- `raster_clip_rect/point_in/selftest`; yalnızca `stdint.h`.

## 35.3 Mantık (`kernel/drivers/raster.c`)
- int64 ara hesap + 1 derleme-zamanı kanıtı; sözleşmeler `REQUIRE` ile.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 35.4 Entegrasyon
- `gfx_fill_rect` satır-içi kırpma yerine çağrı kullanır (döngü aynı).
- `kernel.c` `[35]` bloğu: `raster` öztest kaydı + sonuç basımı.

## 35.5 Host test (`tests/host/test_raster35.c`, `make test-raster35`)
- GERÇEK `kernel/drivers/raster.c` derlenir: TUMU PASS (13 kontrol:
  negatif, INT_MIN/MAX, taşma, sınır).

## 35.6 Derleme + eşdeğerlik
- `KERNEL_SRCS` += `kernel/drivers/raster.c`; `make build/kernel.bin`
  temiz, 3 sembol ELF'te.
- Diferansiyel kanıt (/tmp, tek seferlik): eski/yeni kırpma 32 vakada
  0 fark (aralık-içi girdilerde birebir eşdeğer).
- Not: QEMU açılışı `[2A]` APIC init'te ölüyor (0xFEE00020 sayfa hatası);
  repo bayraklarıyla da aynı — 22–35'ten bağımsız mevcut sorun
  (tüm 22–35 kodu bundan sonra çalışır). Grafik regresyonu diferansiyel
  test + host testle kapatıldı.

## 35.7 Kod inceleme
- [x] Eski/yeni mantık birebir (diferansiyel kanıtlı)
- [x] Yüzey boyutu 2^31 üstü reddedilir (cast öncesi koruma)
- [x] NULL çıkış reddedilir
- [x] `raster.c` bağımlılığı: `stdint.h` + `verify.h`

## 35.8 Güvenlik denetimi notu
- Düşman koordinatlar OOB yazmaya dönüşemez (kırpma + surf_put çifti).
- Kalan: `gfx_blit` hızlı yolu kendi kırpmasını yapar (ayrı denetimde).

## 35.9 Dokümantasyon
- Bu dosya + header içi sözleşmeler.

## 35.10 Sürüm
- Versiyon: 35.0.0. `make test-raster35` + çekirdek derlemesi yeşil.
- Dosyalar: `include/drivers/raster.h`, `kernel/drivers/raster.c`,
  `tests/host/test_raster35.c`, `docs/35_raster.md`.
