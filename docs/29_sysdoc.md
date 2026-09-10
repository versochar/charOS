# 29 — Makine-Okunur Syscall Belgeleri (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (29.1–29.10).
Tespit: syscall'ların açıklaması yalnızca header yorumlarındaydı;
kullanıcı alanı ve açılış öztesti programatik erişemiyordu.

## 29.1 Tasarım
- Statik nr-sıralı tablo (ad + kısa açıklama), sınırlı kopyalama.
- Yeni syscall'lar: `SYS_DOCNAME=166`, `SYS_DOCDESC=167`.
- Tablo kendini de belgeler (166/167 kayıtlı).

## 29.2 API (`include/core/doc.h`)
- `doc_count/name/desc/copy/selftest`; yalnızca `stdint.h`.

## 29.3 Mantık (`kernel/core/doc.c`)
- 61 kayıt (0–167 arası tanımlı numaralar) + 1 derleme-zamanı kanıtı.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 29.4 Entegrasyon
- `syscall.c`: `sys_docname/desc_wrap` (yığın tamponu + `copy_to_user`) + tablo.
- `kernel.c` `[29]` bloğu: `doc_selftest` + kayıt defterine `doc` kaydı.
- 29.4-fix: 166/167 ilk revizyonda tabloda yoktu (host test yakaladı).

## 29.5 Host test (`tests/host/test_doc29.c`, `make test-doc29`)
- GERÇEK `kernel/core/doc.c` derlenir: TUMU PASS (13 kontrol).

## 29.6 Derleme
- `KERNEL_SRCS` += `kernel/core/doc.c`; `make build/kernel.bin` temiz,
  5 `doc_*` sembolü ELF'te.

## 29.7 Kod inceleme
- [x] Tablo nr-sıralı (öztest kanıtlar), ad/açıklama boş değil
- [x] Kopya NUL paylı, taşma reddedilir
- [x] Sarmalayıcılar tamponu 32/64 baytla sınırlar, kullanıcı buf denetlenir
- [x] Bilinmeyen nr NULL döner (sarmalayıcı -1'e çevirir)

## 29.8 Güvenlik denetimi notu
- Salt-okunur tablo; kullanıcı girdisi yalnızca nr (aralık denetimli).
- Yığın tamponu sabit boyutlu, `copy_to_user` öncesi `is_user_buf_valid`.

## 29.9 Dokümantasyon
- Bu dosya + tablo içi açıklamalar (kullanıcı `man` altyapısının temeli).

## 29.10 Sürüm
- Versiyon: 29.0.0. `make test-doc29` + çekirdek derlemesi yeşil.
- Dosyalar: `include/core/doc.h`, `kernel/core/doc.c`,
  `tests/host/test_doc29.c`, `docs/29_sysdoc.md`.
