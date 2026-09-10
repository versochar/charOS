# 27 — Derleme-Zamanı Sürüm Bilgisi (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (27.1–27.10).
Tespit: `VERSION` dosyası vardı ama çekirdek kendi sürümünü bilmiyordu.

## 27.1 Tasarım
- Sürüm dizgisi + kısa commit hash'i salt-okunur veride yaşar.
- Değerler derleme anında Makefile'dan `-D` ile gelir; tanımsızsa
  geliştirme varsayılanı (`charOS dev` / `nogit`).
- Yalnızca `version.o` etkilenir (hedefe-özel CFLAGS).

## 27.2 API (`include/core/version.h`)
- `version_string()`, `version_commit()`; yalnızca bildirim.

## 27.3 Mantık (`kernel/core/version.c`)
- `#ifndef` geri dönüşlü iki statik dizgi; aynı dosya çekirdekte ve host'ta.

## 27.4 Entegrasyon
- Makefile: `CHAROS_VER` (VERSION ilk satırı) + `CHAROS_COMMIT`
  (git short hash, yoksa `nogit`); `build/core/version.o` hedefine `-D`.
- `kernel.c` `[27]` bloğu açılışta bandı basar (VGA + serial).
- 27.4-fix: ilk `-D` alıntısı kabukta çözülüyordu (derleme hatası);
  tek-tırnaklı forma düzeltildi.

## 27.5 Host test (`tests/host/test_version27.c`, `make test-version27`)
- GERÇEK `kernel/core/version.c` derlenir: TUMU PASS (4 kontrol).

## 27.6 Derleme
- `KERNEL_SRCS` += `kernel/core/version.c`; `make build/kernel.bin` temiz.
- Doğrulama: `strings build/kernel.elf` içinde `charOS v0.1.1` ve o anki
  commit hash'i; `version_*` sembolleri ELF'te.

## 27.7 Kod inceleme
- [x] `-D` değerleri yalnızca `version.o`'yu etkiler (diğer hedefler temiz)
- [x] VERSION/git yokluğunda geri dönüş tanımlı (her ortamda derlenir)
- [x] `version.c` bağımlılığı: yalnızca bildirim başlığı
- [x] Salt-okunur veri (çalışma-zamanı yazma yok)

## 27.8 Güvenlik denetimi notu
- Sürüm bandı bilgi sızıntısı sayılmaz (zaten ISO/VERSION herkese açık).
- Geri dönüş değerleri sabit — fingerprint'e yeni entropi eklemez.

## 27.9 Dokümantasyon
- Bu dosya + header içi açıklamalar.

## 27.10 Sürüm
- Versiyon: 27.0.0. `make test-version27` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/version.h`, `kernel/core/version.c`,
  `tests/host/test_version27.c`, `docs/27_buildver.md`.
