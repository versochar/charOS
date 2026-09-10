# 26 — Profilleyici (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (26.1–26.10).
Tespit: `perf.c` 24 satırdı, yalnızca açılış yazıları — yeniden kullanılabilir
ölçüm ilkeli yoktu.

## 26.1 Tasarım
- Sabit slot tablosu (tahsis yok, 16 slot), min/max/toplam/sayaç.
- Tick kaynağı enjekte edilir: çekirdekte rdtsc, host'ta sahte sayaç.
- u64 çıkarma ile sarmal güvenli; iç içe ölçüm reddedilir.

## 26.2 API (`include/core/prof.h`)
- `prof_init/set_tick/tick_rdtsc/begin/end/read/reset`; yalnızca `stdint.h`.

## 26.3 Mantık (`kernel/core/prof.c`)
- Saf sayaç kodu + 1 derleme-zamanı kanıtı; sözleşmeler `REQUIRE` ile
  (0xD10x kodları). Aynı dosya çekirdekte ve host'ta derlenir.

## 26.4 Entegrasyon
- `kernel.c` `[26]` bloğu: `verify_selftest` ×32 ve syslog turu ×8 ölçülür,
  ortalama seri+VGA'ya basılır, tutarlılık (`min<=avg<=max`, sayaç) denetlenir.
- Sıcak yola dokunulmaz (yalnızca açılış ölçümü).

## 26.5 Host test (`tests/host/test_prof26.c`, `make test-prof26`)
- GERÇEK `kernel/core/prof.c` derlenir: TUMU PASS (18 kontrol, sahte tick).

## 26.6 Derleme
- `KERNEL_SRCS` += `kernel/core/prof.c`; `make build/kernel.bin` temiz,
  7 `prof_*` sembolü ELF'te.

## 26.7 Kod inceleme
- [x] Slot/id sınırları her fonksiyonda denetlenir
- [x] Tick yokken ölçüm reddedilir (sessiz çöp yok)
- [x] İç içe `begin` ve eşleşmesiz `end` reddedilir
- [x] `prof.c` bağımlılığı: `stdint.h` + `verify.h`
- [x] 32-bit baskıda u64 ortalama daralması bilinçli (açılış raporu)

## 26.8 Güvenlik denetimi notu
- Profiler fail-observable: hata -1 döner, sayaç kirlenmez.
- Saldırgan kontrolünde tick yok (çekirdek rdtsc sabitler).

## 26.9 Dokümantasyon
- Bu dosya + header içi API açıklamaları.

## 26.10 Sürüm
- Versiyon: 26.0.0. `make test-prof26` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/prof.h`, `kernel/core/prof.c`,
  `tests/host/test_prof26.c`, `docs/26_prof.md`.
