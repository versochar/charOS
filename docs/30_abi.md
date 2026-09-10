# 30 — API Kararlılık Bekçisi (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (30.1–30.10).
Tespit: syscall numaraları yalnızca header yorumlarındaydı; yeniden
numaralandırma veya belgesiz kayıt hiçbir kapıya takılmıyordu.

## 30.1 Tasarım
- Mühür: 8 kritik nr->ad eşleşmesi değişemez (bilinçli karar gerektirir).
- Kapsama: kayıtlı her syscall'ın belge kaydı olmalı, açılışta sayılır.

## 30.2 API (`include/core/abi.h`)
- `abi_frozen_check/selftest` + `syscall_abi_check` (tanım syscall.c'de).

## 30.3 Mantık (`kernel/core/abi.c`)
- Mühür tablosu + ad karşılaştırma + 1 derleme-zamanı kanıtı.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 30.4 Entegrasyon
- `syscall.c`: `syscall_abi_check()` tabloyu tarar, belgesiz sayar.
- `kernel.c` `[30]` bloğu: mühür + kapsama koşar, `[30] abi [PASS/FAIL]`
  basar; `abi` öztest defterine kaydolur.

## 30.5 Host test (`tests/host/test_abi30.c`, `make test-abi30`)
- GERÇEK `kernel/core/abi.c` derlenir: TUMU PASS (5 kontrol).

## 30.6 Derleme
- `KERNEL_SRCS` += `kernel/core/abi.c`; `make build/kernel.bin` temiz,
  3 sembol ELF'te.

## 30.7 Kod inceleme
- [x] Mühür yalnızca ad karşılaştırır (sayısal kayma yakalanır)
- [x] Kapsama O(256) açılışta bir kez (sıcak yol etkilenmez)
- [x] `abi.c` bağımlılığı: `stdint.h` + `verify.h` + `doc.h`
- [x] Yeni syscall ekleyen geliştirici belgeyi de eklemek zorunda (kapı)

## 30.8 Güvenlik denetimi notu
- Belgesiz syscall = denetlenmemiş saldırı yüzeyi; kapı bunu açılışta
  görünür kılar. Mühür, sessiz ABI kırılmasını engeller.

## 30.9 Dokümantasyon
- Bu dosya + header içi sözleşmeler.

## 30.10 Sürüm
- Versiyon: 30.0.0. `make test-abi30` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/abi.h`, `kernel/core/abi.c`,
  `tests/host/test_abi30.c`, `docs/30_abi.md`.
