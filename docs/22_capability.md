# 22 — Capability & Permissions (gerçek çekirdek kodu)

Yeni şema: harf yok, tek sayıda 10 aşama (22.1–22.10).
Stub/davranış modeli yok: `kernel/process/cap.c` gerçek kod, çekirdeğe bağlı.

## 22.1 Tasarım
- Adlandırılmış capability bitleri, mevcut ABI ile uyumlu.
- Enforcement noktaları: setuid, setgid, setgroups, blkwrite.
- İlke: root bootstrap'ta CAP_ALL, fork miras alır, root bırakılınca düşer.

## 22.2 API (`include/process/cap.h`)
- Bitler: CHOWN=0, DAC_OVERRIDE=1, DAC_READ_SEARCH=2, FOWNER=3, FSETID=4,
  KILL=5, SETGID=6, SETUID=7, SYS_RAWIO=8, SYS_ADMIN=9, NET_ADMIN=10,
  SYSLOG=11, SYS_MODULE=12. CAP_ALL=0x1FFF.
- `core/syscall.h` içindeki dağınık CAP tanımları buraya taşındı
  (tek doğruluk kaynağı). `chfs.c` ve `userprog.c` değerleri değişmedi.

## 22.3 Mantık (`kernel/process/cap.c`)
- `cap_valid/has/grant/revoke/drop_all/is_subset/allow_only` + 16 girişli
  denetim halkası (`cap_audit`, `cap_audit_read`).
- task yapısına bağımlı değil: aynı dosya çekirdekte ve host testinde derlenir.

## 22.4 Task entegrasyonu
- `task.c` (`fd_table_init` ortak yolu): yeni task CAP_ALL + ngroups=0.
- `fork.c`: `child->caps = parent->caps` (önceden memset ile sıfırlanıyordu).
- Düzeltilen gerçek bug'lar:
  - B1: caps hiçbir yerde init edilmiyordu (root bile caps=0 ile çalışıyordu).
  - B2: fork yetkileri miras bırakmıyordu.

## 22.5 Enforcement (`kernel/core/syscall.c`)
- `setuid`: değişiklik CAP_SETUID ister; root→non-root geçişinde caps düşer.
- `setgid`/`setgroups`: CAP_SETGID ister (eski uid==0 kontrolü yerine).
- `blkwrite`: CAP_SYS_RAWIO ister (önceden yetki denetimi YOKTU — B4).
- `capset`: root geçersiz biti reddeder; diğerleri `cap_allow_only` ile alt küme.
- Mevcut userprog testleri (fork-child setuid, 20C caps) aynı kalır, geçer.

## 22.6 Host test (`tests/host/test_cap22.c`, `make test-cap22`)
- GERÇEK `kernel/process/cap.c` derlenir, sonuç: TUMU PASS (24 kontrol).

## 22.7 Derleme
- `KERNEL_SRCS` += `kernel/process/cap.c`; `make build/kernel.bin` temiz
  (uyarı yok), `cap_*` sembolleri ELF'te doğrulandı (9 sembol).
- 22.7-fix: ilk taslak bitleri `syscall.h` ile çakışıyordu (derleyici uyarısı
  yakaladı); mevcut ABI kazanacak şekilde düzeltildi (B5).

## 22.8 Kod inceleme
- [x] Bit değerleri mevcut ABI ile aynı (chfs.c, userprog.c etkilenmez)
- [x] Hata kodları tutarlı (0/-1, yükseltmede istek reddedilir)
- [x] NULL kontrolleri tam
- [x] `cap.c` bağımlılıkları: yalnızca `stdint.h` (freestanding-safe)
- [x] Mevcut testler (userprog 20C, fork setuid) korunur

## 22.9 Güvenlik denetimi
- Priv-esc senaryoları `test_cap22` içinde: düşmüş görev yükselemez,
  key yokken rawio yok, wipe/yetki-düşürme sonrası erişim reddedilir.
- Düzeltilen vektör (B3): root→non-root setuid sonrası caps elde kalıyordu;
  artık `cap_drop_all` çalışır.
- Kalan not: `exec` dosya-capability (setuid-bit) uygulamaz — takip işi.

## 22.10 Sürüm
- Versiyon: 22.0.0. `make test-cap22` + `make build/kernel.bin` yeşil.
- Dosyalar: `include/process/cap.h`, `kernel/process/cap.c`,
  `tests/host/test_cap22.c`, `docs/22_capability.md`.
