# 31 — Güç Muhasebesi (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (31.1–31.10).
Tespit: boşta-görev çıplak `hlt` dönüyordu, boşta/meşgul ayrımı sayılmıyordu.

## 31.1 Tasarım
- Timer IRQ her tick'te boşta/meşgul sayar; politika danışımsal
  (PERF/BALANCED/SAVER), donanım P-state köprüsü takip işi.
- Yüzde tamsayı hesabı, sayaçsız %0.

## 31.2 API (`include/core/power.h`)
- `init/set_policy/get_policy/note_tick/idle_ticks/busy_ticks/idle_pct/selftest`.

## 31.3 Mantık (`kernel/core/power.c`)
- Saf sayaç + politika + 1 derleme-zamanı kanıtı; sözleşmeler `REQUIRE` ile.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 31.4 Entegrasyon
- `task.c`: `task_on_idle()` (o an boşta-görevde miyiz).
- `timer.c`: IRQ'da `power_note_tick(task_on_idle())` (ucuz: 2 sayaç).
- `kernel.c` `[31]` bloğu: init + politika + `power` öztest kaydı.
- Not: öztest sayaçları sıfırlar (belgelendi); IRQ saymaya devam eder.

## 31.5 Host test (`tests/host/test_power31.c`, `make test-power31`)
- GERÇEK `kernel/core/power.c` derlenir: TUMU PASS (9 kontrol).

## 31.6 Derleme
- `KERNEL_SRCS` += `kernel/core/power.c`; `make build/kernel.bin` temiz,
  9 sembol ELF'te.

## 31.7 Kod inceleme
- [x] Politika aralığı korunur (geçersiz değer reddedilir)
- [x] Bölme-sıfır yok (sayaçsız %0)
- [x] IRQ bağlamı güvenli (kilit yok, yalnızca sayaç)
- [x] `power.c` bağımlılığı: `stdint.h` + `verify.h`
- [x] u32 sayaç sarmalı ~497 günde bir (100Hz, belgelendi)

## 31.8 Güvenlik denetimi notu
- Muhasebe salt-gözlem: zamanlama yan-kanalına yeni kaynak eklemez
  (tick zaten herkese açık).
- Politika değişimi ayrıcalık gerektirmez (v1; yetkilendirme takip işi).

## 31.9 Dokümantasyon
- Bu dosya + header içi açıklamalar.

## 31.10 Sürüm
- Versiyon: 31.0.0. `make test-power31` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/power.h`, `kernel/core/power.c`,
  `tests/host/test_power31.c`, `docs/31_power.md`.
