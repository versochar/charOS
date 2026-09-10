# 23 — Sandbox & Isolation (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (23.1–23.10).
seccomp64 stub'undan farklı: global filtre değil, görev başına zorlanan filtre.

## 23.1 Tasarım
- Görev başına 256 bit syscall maskesi + `sb_on` bayrağı.
- Varsayılan: `sb_on=0` (izinli), maske tam-serbest. Kilitlenen görev maskeye uyar.
- Fork mirası; geri dönüşsüz sıkılaştırma (SB_ON).

## 23.2 API (`include/process/sandbox.h`, `SYS_SB_*`)
- `sb_allow_all/lockdown/allow/deny/check`; `SB_MASK_WORDS=8`.
- Yeni syscall'lar: `SYS_SB_ALLOW=163`, `SYS_SB_DENY=164`, `SYS_SB_ON=165`.

## 23.3 Mantık (`kernel/process/sandbox.c`)
- Saf bitmask kodu, task-bağımsız; aynı dosya çekirdekte ve host'ta derlenir.

## 23.4 Task entegrasyonu
- `task.h`: `sb_mask[8]` + `sb_on` alanları.
- `task.c`: yeni task serbest maske + `sb_on=0`.
- `fork.c`: maske + bayrak miras alınır.

## 23.5 Enforcement (`kernel/core/syscall.c`)
- `syscall_handler`: `sb_on` açık ve bit kapalıysa `-1` dön, dispatch etme.
- `SB_ALLOW/DENY/ON` sarmalayıcıları + tablo kayıtları.
- Mevcut testler etkilenmez (hiçbir görev varsayılanla kilitlenmez).

## 23.6 Host test (`tests/host/test_sandbox23.c`, `make test-sandbox23`)
- GERÇEK `kernel/process/sandbox.c` derlenir, sonuç: TUMU PASS (14 kontrol).

## 23.7 Derleme
- `KERNEL_SRCS` += `kernel/process/sandbox.c`; `make build/kernel.bin` temiz
  (uyarı yok), `sb_*` sembolleri ELF'te doğrulandı (5 sembol).

## 23.8 Kod inceleme
- [x] Maske sınırları (nr<256) her fonksiyonda denetlenir
- [x] NULL maske güvenli (allow_all/lockdown no-op, check=deny)
- [x] `sandbox.c` bağımlılığı: yalnızca `stdint.h`
- [x] Handler ek yükü: `sb_on==0` iken tek dal (sıcak yolda ucuz)
- [x] Mevcut syscall numaraları değişmedi (163+ yeni)

## 23.9 Güvenlik denetimi
- Kilitli görev `SB_ON` sonrası yasaklı çağrıyı çalıştıramaz (handler reddi).
- Maske görev-dışı değiştirilemez (yalnızca kendi `task_current` maskesi).
- Not: `SB_ALLOW` ile kendini yeniden serbest bırakma mümkün (tasarım:
  tek yönlü değil, seçici filtre). Tam tek-yön kilit takip işi.

## 23.10 Sürüm
- Versiyon: 23.0.0. `make test-sandbox23` + `make build/kernel.bin` yeşil.
- Dosyalar: `include/process/sandbox.h`, `kernel/process/sandbox.c`,
  `tests/host/test_sandbox23.c`, `docs/23_sandbox.md`.
