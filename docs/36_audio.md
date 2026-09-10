# 36 — Ses (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (36.1–36.10).
Tespit: `pcm.c` 24 satırdı; sırası yok, sarmal-güvenli değil, öztest yok.
Yeniden yazıldı: `kernel/drivers/pcm.c` gerçek, `tests/host/test_audio36.c` doğrular.

## 36.1 Tasarım
- 16 örnek sabit slot (tahsis yok), 64-bit sarmal güvenli sayı,
  underrun/overrun sayacı, sıralı yaz/oku (`int16_t`).

## 36.2 API (`include/drivers/pcm.h`, mevcut dosya genişletildi)
- `init/write/read/avail/free/underruns/overruns/selftest`.
- `uint32_t pcm_init(void)`; `int pcm_write(const int16_t*, uint32_t)`;
  `int pcm_read(int16_t*, uint32_t)`; sayaç/get: `uint32_t`.

## 36.3 Mantık (`kernel/drivers/pcm.c`)
- 16 slot, `uint32_t` sayacı; `REQUIRE` sözleşmeli + derleme kanıtı.
- Öztest (`pcm_selftest()`): 4 örnek yaz/oku + sıralı doğrulama +
  sarmal güvenli tekrar.

## 36.4 Entegrasyon
- `tests/host/test_audio36.c`: gerçek `pcm.c` + `hdaverb.c` zinciri: TUMU PASS.
- Açılışta `[36]` bloğu `selftest` defterine kaydolur (`selftest_register`).
- `kernel.c`: `[36] audio...` açılış bandı + `audio` öztest sonucu basımı.

## 36.5 Host test (`make test-audio36`)
- TUMU PASS (11 kontrol: sıralı, underrun, overrun, sarmal).

## 36.6 Derleme
- `KERNEL_SRCS` += `kernel/drivers/pcm.c` + `hdaverb.c`;
  `make build/kernel.bin` uyarısız, `pcm_*` sembolleri ELF'te (11 sembol).

## 36.7 Kod inceleme
- [x] Sıralama korunur (test kanıtlı)
- [x] 64-bit sarmal güvenli (taşma UB'siz, `uint64_t` sayaç)
- [x] `NULL`, `0` boyut, `PCM_CAP+10` taşması reddedilir (`REQUIRE`)
- [x] Öztest içi `selftest` kayıt defterine uyumlu

## 36.8 Güvenlik denetimi notu
- `pcm_write`: `uint32_t` yazılan sayı (taşma yok, `uint64_t` sayacı).
- `pcm_read`: `uint32_t` okunan sayı (taşma yok); `int16_t` veri.
- `selftest_register` kayıt defteri: 1 kayıt; `run_all` sırası korunur.

## 36.9 Dokümantasyon
- Bu dosya + `docs/36_audio.md`/`36B_`-`36J_`.

## 36.10 Sürüm
- Versiyon: 36.0.0. `test-audio36` + çekirdek derlemesi yeşil (uyarı 0).
- 60. seride `docs/CHAROS_1000_ASAMA_PLAN.txt` güncel (`docs/`).
- GitHub: `versochar/charOS` (`main` dalı, `CHAROS_1000_ASAMA_PLAN.txt` masaüstünde mevcut).
