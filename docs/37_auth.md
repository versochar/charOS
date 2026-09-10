# 37 — Kimlik Doğrulama (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (37.1–37.10).
Tespit: `syscall.c`'te `SYS_AUTH` eklenmişti ama gerçek `auth_check()`
bağlantısı eksikti; `kernel/core/auth.c` yeni yazıldı.

## 37.1 Tasarım
- Token türetimi: sabit seed (`DEADBEEF`) + ad uzunluğu → karışım; doğrulama karşılaştırma.
- Kanca: `syscall_handler` auth çağrısını doğrudan `auth_check`'e geçirir; kanca `syslog`'a düşmez (fail-closed, hata sessiz).

## 37.2 API (`include/core/auth.h`)
- `auth_init/check/token_gen`; yalnızca `stdint.h`.

## 37.3 Mantık (`kernel/core/auth.c`)
- Gerçek token türetimi + karşılaştırma + sözleşme `REQUIRE` entegrasyonu.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 37.4 Entegrasyon
- `syscall.c`: `sys_auth_wrap` tanımlı (token param, 0 ok / -1 ret).
- `syscall_init`: `SYS_AUTH` tablosu güncel.
- `auth.c`: `REQUIRE` sözleşmeleri ile (hata sessiz -1).

## 37.5 Host test (`tests/host/test_auth37.c`, `make test-auth37`)
- GERÇEK `auth.c` derlenir: PASS (token türetimi deterministik, doğrulama 0 ok / -1 ret).

## 37.6 Derleme
- `KERNEL_SRCS` += `kernel/core/auth.c`; `make build/kernel.bin` temiz,
  `auth_*` sembolleri ELF'te.

## 37.7 Kod inceleme
- [x] Token türetimi sabit seed + uzunluğa bağlı (deterministik)
- [x] `REQUIRE` sözleşmeleri parametre hatalarını sessiz -1 yapar
- [x] `sys_auth_wrap` doğrudan `auth_check` çağırır (kanca yok, fail-closed)
- [x] Mevcut syscall numaraları değişmedi (`SYS_AUTH=168` son)
- [x] `auth.c` bağımlılığı: `stdint.h` + `verify.h`

## 37.8 Güvenlik denetimi notu
- Token türetimi sabit seed kullanır; gerçek kripto değil ama
  karşılaştırma hatası sessiz -1 (fail-observable).
- Kanca yok; auth hata sessizce syscall'a yansır — kullanıcı uygulaması
  ret -1 alır (uygun güvenlik politikasına göre işlenmeli).

## 37.9 Dokümantasyon
- Bu dosya + `core/auth.h` içi açıklamalar.

## 37.10 Sürüm
- Versiyon: 37.0.0. `make test-auth37` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/auth.h`, `kernel/core/auth.c`,
  `tests/host/test_auth37.c`, `docs/37_auth.md`.
