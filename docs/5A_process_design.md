# 5A - Process & Threading Tasarım

## Amaç
charOS için hafif process/thread modeli: PID namespace uyumlu, öncelikli yapı, güvenli exit/wait.

## Kararlar
- **PID yönetimi**: monoton artan PID, reuse yok (v1). `proc64_spawn` yeni PID döner.
- **Durum makinesi**: READY -> RUNNING -> ZOMBIE -> FREE. `exit` ZOMBIE yapar, `wait` FREE yapar.
- **Thread**: v1'de tek thread/process; `proc64_thread_add` plan olarak rezerve.
- **Güvenlik**: geçersiz PID'de -1, double-exit reddedilir, wait yalnızca ZOMBIE'de başarılı.

## Bileşenler
- `kernel/arch/x86_64/proc64.c` (host-test davranış modeli)
- `include/arch/x86_64/proc.h`
- Gerçek scheduler entegrasyonu (`process/` dizini) plan olarak rezerve; mevcut çekirdek bozulmaz.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
5B API spesifikasyonu.
