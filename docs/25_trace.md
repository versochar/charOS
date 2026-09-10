# 25 — Seviyeli İzleme (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (25.1–25.10).
Tespit: syslog ham metindi; seviye, etiket, eşik filtresi ve sayaç yoktu.

## 25.1 Tasarım
- 5 seviye (DEBUG/INFO/WARN/ERROR/FATAL), çalışma-zamanı eşik, seviye sayaçları.
- Arka uç kanca ile enjekte edilir; çekirdekte syslog, yoksa düşürme.

## 25.2 API (`include/core/trace.h`)
- `trace_init/set_backend/set_level/event/count`; yalnızca `stdint.h`.

## 25.3 Mantık (`kernel/core/trace.c`)
- Saf filtre + sayaç + 1 derleme-zamanı kanıtı (seviye sıralaması).
- Aynı dosya çekirdekte ve host'ta derlenir.

## 25.4 Entegrasyon
- `kernel.c`: `trace_kbackend` (syslog'a yazar), açılışta `trace_init` +
  backend kurulumu; `verify_khook` artık doğrudan `syslog_puts` yerine
  `trace_event(WARN, "verify", ...)` kullanıyor (tek log hunisi).
- Sıralama önemli: backend önce kurulur, verify kancası ona yazar.

## 25.5 Host test (`tests/host/test_trace25.c`, `make test-trace25`)
- GERÇEK `kernel/core/trace.c` derlenir: TUMU PASS (13 kontrol).

## 25.6 Derleme
- `KERNEL_SRCS` += `kernel/core/trace.c`; `make build/kernel.bin` temiz,
  `trace_*` sembolleri ELF'te (6 sembol). `test-verify24` regresyonu yeşil.

## 25.7 Kod inceleme
- [x] Eşik dışı seviye reddedilir, sayaç kirlenmez
- [x] NULL tag/msg ve backendsiz çağrı güvenli düşer
- [x] Geçersiz `set_level` mevcut eşiği korur
- [x] `trace.c` bağımlılığı: yalnızca `stdint.h` (+`verify.h` kanıt makrosu)

## 25.8 Güvenlik denetimi notu
- Trace fail-observable: düşen olay sayılmaz, sessiz kayıp yok (eşik bilinçli).
- Backend NULL iken olay üretilmez — açılış sırası (25.4) bunu garanti eder.

## 25.9 Dokümantasyon
- Bu dosya + header içi seviye açıklamaları.

## 25.10 Sürüm
- Versiyon: 25.0.0. `make test-trace25` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/trace.h`, `kernel/core/trace.c`,
  `tests/host/test_trace25.c`, `docs/25_trace.md`.
