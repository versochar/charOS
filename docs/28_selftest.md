# 28 — Öztest Kayıt Defteri (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (28.1–28.10).
Tespit: onlarca `*_selftest()` dağınık çağrılıyordu; kayıt, sıra ve özet yoktu.

## 28.1 Tasarım
- Sabit tablo (32 giriş), ad+fn kaydı, kayıt-sırası koşum, FAIL sayacı.
- Sözleşme: fn 0=PASS. Kopya/boş/taşıma reddedilir.

## 28.2 API (`include/test/selftest.h`)
- `selftest_init/register/run_all/count`; yalnızca `stdint.h`.

## 28.3 Mantık (`kernel/test/selftest.c`)
- Dış bağımlılıksız ad karşılaştırma + 1 derleme-zamanı kanıtı.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 28.4 Entegrasyon
- `kernel.c` `[28]` bloğu: `verify` + `syslog` öztestlerini kaydedip
  toplu koşar, `[28] selftests [PASS/FAIL]` basar.
- Dağınık çağrılar duruyor (geriye uyumluluk); kayıt defteri dopluyor.

## 28.5 Host test (`tests/host/test_selftest28.c`, `make test-selftest28`)
- GERÇEK `kernel/test/selftest.c` derlenir: TUMU PASS (13 kontrol).

## 28.6 Derleme
- `KERNEL_SRCS` += `kernel/test/selftest.c`; `make build/kernel.bin` temiz,
  `selftest_*` sembolleri ELF'te.

## 28.7 Kod inceleme
- [x] NULL/boş/kopya/taşıma reddi tam
- [x] Koşum sırası kayıt sırası (deterministik)
- [x] `selftest.c` bağımlılığı: `stdint.h` + `verify.h`
- [x] FAIL sayacı int sarmalı pratikte imkânsız (32 giriş)

## 28.8 Güvenlik denetimi notu
- Kayıt defteri yalnızca açılışta yazılır; çalışma-zamanı kaydı yok
  (kötü amaçlı modül enjeksiyon yüzeyi açılmaz).
- Test fn'leri güvenilir kod olmak zorunda (belgelendi).

## 28.9 Dokümantasyon
- Bu dosya + header içi sözleşme açıklamaları.

## 28.10 Sürüm
- Versiyon: 28.0.0. `make test-selftest28` + çekirdek derlemesi yeşil.
- Dosyalar: `include/test/selftest.h`, `kernel/test/selftest.c`,
  `tests/host/test_selftest28.c`, `docs/28_selftest.md`.
