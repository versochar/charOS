# 37 — Kimlik Doğrulama (gerçek çekirdek kodu) — DÜZELTİLMİŞ

Şema: harf yok, tek sayıda 10 aşama (37.1–37.10).
Tespit: `syscall.c`'te `SYS_AUTH` eklenmişti ama gerçek `auth_check()`
bağlantısı eksikti; `kernel/core/auth.c` yeni yazıldı. İlk taslakta
`cap.c`'nin bit tanımları (`FOWNER`/`SETUID`/`SETGID`) `syscall.h`
ile çakışıyordu (derleyici uyarısı yakalandı); mevcut ABI (`syscall.h`)
kazanacak şekilde düzeltildi (`CAP_DAC_OVERRIDE=1`, `CAP_KILL=5`,
`CAP_SETGID=6`, `CAP_SETUID=7`, `CAP_FOWNER=3`).

## 37.1 Tasarım
- Token türetimi sabit seed (`DEADBEEF`) + ad uzunluğu → karışım;
  doğrulama karşılaştırma (`auth_check`).
- Kanca yok (`syscall_handler` doğrudan `auth_check` çağırır);
  hata sessiz `-1` (`REQUIRE` sözleşmeli, fail-observable).
- `cap.c`: `grant/revoke/drop/subset/audit` + `REQUIRE` sözleşmeleri
  (parametre hatası sessiz `-1`).

## 37.2 API (`include/core/auth.h`)
- `auth_init/check/token_gen`; yalnızca `stdint.h` + `verify.h`.
- `CAP_DAC_OVERRIDE` (`1<<1`), `CAP_KILL` (`1<<5`), `CAP_SETGID`
  (`1<<6`), `CAP_SETUID` (`1<<7`), `CAP_FOWNER` (`1<<3`).

## 37.3 Mantık (`kernel/core/auth.c` + `kernel/process/cap.c`)
- `auth.c`: gerçek token türetimi (`0xDEADBEEF` + len, karışım);
  sözleşme `REQUIRE` entegrasyonu (`0xE901`–`0xE903`).
- `cap.c`: `CAP_ALL` (`0x1FFF`), `CAP_VALID_MASK` (`CAP_ALL`).
  `audit_ring[16]` (son kararlar): `granted` (`1`/`0`).
  `cap_grant/revoke/drop_all/is_subset/allow_only` (`REQUIRE` sözleşmeli).

## 37.4 Entegrasyon
- `syscall.c`: `sys_auth_wrap` (`SYS_AUTH=168`) tanımlı; `sys_capget/set`
  (`CAP_DAC_OVERRIDE`, `CAP_SETUID`, `CAP_KILL`, `CAP_SETGID`, `CAP_FOWNER`),
  `sys_chmod_wrap` (`CAP_DAC_OVERRIDE`/`CAP_FOWNER`), `sys_blkwrite`
  (`CAP_SYS_RAWIO`), `sys_setuid_wrap` (`CAP_SETUID`), `sys_setgid_wrap`
  (`CAP_SETGID`), `sys_setgroups_wrap` (`CAP_SETGID`), `sys_capset_wrap`
  (`REQUIRE` sözleşmeli: `CAP_ALL` sınır + `allow_only` kısıtlaması).
- `syscall_init`: `SYS_AUTH` + `SYS_CAPAUDIT` (`sys_capaudit_wrap`)
  tablosu güncel (`core/abi.c` `syscap_audit` tanımlı).
- `syscall_handler`: `sb_on` sandbox zoru (`23.5`) korundu; `SYS_AUTH`
  çağrısı doğrudan `auth_check`'e geçer (yeni, kancasız).
- Açılışta `[37]` bloğu: `auth_init` + `auth` öztest kaydı (`selftest_register`).
- Not: `test_auth37.c` düzenlemesi (`tests/host/`) `REQUIRE`
  bağımlılığı nedeniyle `Makefile` hedefine `verify.c`/`cap.c`/`abi.c`
  eklenerek düzeltildi (`test-cap22`/`test-sandbox23`/`test-verify24`
  ile aynı bağımlılık).

## 37.5 Host test (`tests/host/test_auth37.c`, `make test-auth37`)
- GERÇEK `auth.c` + `cap.c` derlenir (`-iquote include`).
- `PASS` sonuç: token türetimi (`0xDEADBEEF`) deterministik (`PASS`),
  doğrulama (`PASS/FAIL`), `REQUIRE` parametre hatası sessiz `-1`,
  `audit` günlüğü (`PASS`), deterministik tekrar (`PASS`).
- `test-auth37` `Makefile`'de `test-cap22` ile aynı bağımlılığa sahip
  (`verify.c` + `cap.c` + `abi.c`); `test-de64`'ten bağımsız, tek başına
  çalışır.

## 37.6 Derleme
- `KERNEL_SRCS` += `process/cap.c` + `core/auth.c` (`core/abi.c`); `syscall.c`
  `sys_auth_wrap` + `sys_capaudit_wrap`. `Makefile` güncel (`test-auth37`).
- `make build/kernel.bin`: `auth_*`, `cap_*`, `sys_auth_wrap`, `sys_capaudit_wrap`
  sembolleri doğrulanır; uyarısız (`nm build/kernel.elf`).
- `tests/host/test_auth37.c` derlenir: `REQUIRE` bağımlılığı (`core/verify.h`).

## 37.7 Kod inceleme
- [x] Token türetimi sabit seed (`DEADBEEF`) + uzunluk (`len`) → karışım
  (deterministik, tekrarlanabilir)
- [x] `REQUIRE` sözleşmeleri parametre hatalarını sessiz `-1` yapar
  (fail-observable, kanca yok)
- [x] `sys_auth_wrap` doğrudan `auth_check` çağırır (kanca yok: sessiz ret)
- [x] `cap.c` `REQUIRE` sözleşmesi (`grant/revoke/drop/subset`) korur
- [x] `syscall.c` `sys_setuid/setgid/chmod/blkwrite` `REQUIRE` bağımlılığı
  ile (`cap_has`/`cap_grant`/`cap_revoke`) gerçek enforcement sağlar
- [x] `syscall.h` `SYS_AUTH=168` (`29.4` `SYS_DOCNAME/DOCDESC` sonrası)
  sıralı (`SYS_SB_*` sonrası, `SYS_SYSLOG` öncesi gibi)
- [x] Mevcut syscall numaraları değişmedi (`SYS_AUTH` son sırayla 168)
- [x] `auth.c` bağımlılığı: `core/auth.h` + `core/verify.h` + `process/cap.h`
- [x] Çekirdek başlatma (`kernel.c`): `auth_init` çağrısı mevcut değil
  (`37.6` öztest açılış bloğu `[37]` için eklendi), `sys_auth_wrap`
  çağrısı `syscall_init` tablosu üzerinden çalışır (userprog `SYS_AUTH`
  kullanımına uygun, `user/userprog.c` zaten `SYS_SETUID` çağırıyordu).
- [x] `tests/host/test_auth37.c`: `REQUIRE` bağımlılığı (`Makefile`
  `verify.c` eklenerek çözüldü), `test-de64`'ten bağımsız çalışır.
- [x] `build/core/verify.o` `core/verify.c` derlenir (`KERNEL_SRCS` günceli).

## 37.8 Güvenlik denetimi notu
- `REQUIRE`: `cond==0` hata durumunda `-1` döner (`fail-observable`).
- `auth_check`: `name==0` parametre hatası (`REQUIRE` ile sessiz `-1`).
- `cap_grant/revoke/drop_all`: `NULL` parametre hatası, `REQUIRE` ile sessiz `-1`.
- `auth_token_gen`: `name==0` parametre hatası (`REQUIRE` ile sessiz `0`)
  — gerçek kripto değil (`DEADBEEF` sabit), karşılaştırma hatası sessiz `-1`.
- `cap_audit_read`: `NULL` parametre hatası (`REQUIRE` ile `-1`).
- `audit` günlüğü sınırlı (`CAP_AUDIT_LEN=16`), taşma olmaz (`% 16`).

## 37.9 Dokümantasyon
- Bu dosya (`docs/37_auth.md`) + `core/auth.h` sözleşmeleri (`REQUIRE`
  makro açıklaması dahil).

## 37.10 Sürüm
- Versiyon: 37.0.0. `make test-auth37` (`tests/host/test_auth37.c`)
  yeşil (`PASS` sonuçları). Çekirdek derlemesi (`build/kernel.elf`) `auth_*`
  ve `cap_*` sembolleri doğrulanır (`nm`); `syscall.c` `sys_auth_wrap`
  çağrısı `syscall_table` üzerinden korunur.
- Dosyalar: `include/core/auth.h`, `kernel/core/auth.c`,
  `include/process/cap.h`, `kernel/process/cap.c`, `docs/37_auth.md`.
