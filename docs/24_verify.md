# 24 — Doğrulama Altyapısı (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (24.1–24.10).
Tespit: çekirdekte hiç assert/sözleşme mekanizması yoktu (tek "assert"
APIC register adıydı). Bu seri contract katmanını ekler.

## 24.1 Tasarım
- Katmanlar: derleme-zamanı (`STATIC_ASSERT`, gnu99 uyumlu) +
  çalışma-zamanı (`verify_require`: kurtarılabilir -1 + kanca).
- Kanca: çekirdekte syslog'a, host'ta test kancasına bağlanır.
- İlke: sözleşmeler davranışı değiştirmez (hata yolu aynı -1).

## 24.2 API (`include/core/verify.h`)
- `STATIC_ASSERT`, `verify_set_hook`, `verify_require` + `REQUIRE` makrosu,
  `verify_fail_count`, `verify_selftest`. Yalnızca `stdint.h` bağımlılığı.

## 24.3 Mantık (`kernel/core/verify.c`)
- Kanca + sayaç + öztest (kancayı kurar, ihlal üretir, geri yükler).
- Aynı dosya çekirdekte ve host'ta derlenir.

## 24.4 Entegrasyon
- `cap.c`: grant/revoke/drop sözleşmeli + 2 derleme-zamanı kanıtı.
- `sandbox.c`: allow/deny sözleşmeli + 1 derleme-zamanı kanıtı.
- `kernel.c`: açılışta kanca syslog'a bağlanır, `verify_selftest` koşar
  (`[24] verify [PASS]` seri logda).
- 24.4-fix: `test-cap22`/`test-sandbox23` bağlama hatası verdi (yeni
  `verify_require` bağımlılığı) — hedeflere `verify.c` eklendi.

## 24.5 Host test (`tests/host/test_verify24.c`, `make test-verify24`)
- GERÇEK `verify.c` + sözleşmeli `cap.c`/`sandbox.c` derlenir: TUMU PASS.

## 24.6 Derleme
- `KERNEL_SRCS` += `kernel/core/verify.c`; `make build/kernel.bin` temiz,
  `verify_*` sembolleri ELF'te (7 sembol).

## 24.7 Kod inceleme
- [x] Sözleşme makroları yan etkisiz (koşul bir kez değerlendirilir)
- [x] `STATIC_ASSERT` gnu99'da geçerli (negatif dizi boyutu hilesi)
- [x] Kanca NULL-güvenli, sayaç taşması u32 sarmalı (bilinçli)
- [x] Mevcut testler yeşil (test-cap22, test-sandbox23, test-verify24)

## 24.8 Güvenlik denetimi notu
- Sözleşmeler fail-closed değil fail-observable: ihlal -1 döner + loglanır.
  Panik yok (kurtarılabilir kullanıcı girdisi için doğru tercih).
- İhlal kodları deterministik (`0xC10x` cap, `0xB10x` sandbox, `0xE00x` öztest).

## 24.9 Dokümantasyon
- Bu dosya + header içi sözleşme açıklamaları.

## 24.10 Sürüm
- Versiyon: 24.0.0. Üç test hedefi + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/verify.h`, `kernel/core/verify.c`,
  `tests/host/test_verify24.c`, `docs/24_verify.md`.
