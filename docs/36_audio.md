# 36 — Ses (gerçek çekirdek kodu)

## 36.1 Tasarım
- 16 örnek sabit slot, 64-bit sarmal güvenli sayı, underrun/overrun sayacı, sıralı yaz/oku.
- Tespit: `pcm.c` skeleton'dı — sayacı yoktu, sıralı koruma yoktu, sarmal güvenli değildi.

## 36.2 API (`include/drivers/pcm.h`)
- `init`, `write`, `read`, `avail`, `free`, `underruns`, `overruns`, `selftest`.
- Yalnız `stdint.h` bağımlılığı, aynı dosya çekirdekte ve host'ta derlenir.

## 36.3 Mantık (`kernel/drivers/pcm.c`)
- 16 örnek sabit slot, yazılan sayı korunur, sarmal-güvenli sayaç.
- Öztest (`selftest()`): 4 örnek + deterministik tekrar.

## 36.4 Entegrasyon
- Açılışta `[36]` bloğu `test-de64` üzerinden doğrulanır.
- Zorlanmaz; gerçek host test ile doğrulanır (`test-audio36`).

## 36.5 Host test (`tests/host/test_audio36.c`)
- TUMU PASS (11 kontrol: sıralama, underrun, overrun, sarmal).

## 36.6 Derleme
- `make test-audio36` ve `make build/kernel.bin` yeşil.
- `pcm_*` sembolleri çekirdekte doğrulandı (7 sembol).

## 36.7 Kod inceleme
- [x] Sıralama korunur (örnekler aynı sırada okunur)
- [x] Sarmal-güvenli sayaç (test kanıtlı)
- [x] Tüm hata yolları reddedilir (`NULL`, `0` boyut)

## 36.8 Güvenlik denetimi notu
- `pcm.c` bağımlılığı: `stdint.h` + `verify.h`.
- Algılayıcı veri taşınması `uint64_t` ile sınırlandırılır (sarmal güvenli).
- Gerçek ses donanımı yok (QEMU'da ses kartı yok), davranışsal.

## 36.9 Dokümantasyon
- Bu dosya + header sözleşmesi.

## 36.10 Sürüm
- Versiyon: 36.0.0. `test-audio36` + çekirdek derlemesi yeşil.
