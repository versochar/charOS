# 6A - Scheduling & Real-Time Tasarım

## Amaç
charOS için öncelikli round-robin scheduler + real-time sınıfı: deterministik tick, adil seçim, güvenli preemption modeli.

## Kararlar
- **Sınıflar**: RT (prio 0-9, küçük sayı = yüksek öncelik), NORMAL (10-31). v1'de tek CPU varsayımı.
- **Seçim**: en düşük prio numaralı READY görev; eşitlikte FIFO round-robin.
- **Tick**: `sched64_tick()` zaman dilimi bitirir, kuyruk başını sona taşır.
- **Güvenlik**: geçersiz PID/prio reddedilir, boş kuyrukta current=0.

## Bileşenler
- `kernel/arch/x86_64/sched64.c` (host-test davranış modeli)
- `include/arch/x86_64/sched.h`
- Gerçek context-switch (`process/` + timer IRQ) plan olarak rezerve; mevcut çekirdek bozulmaz.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
6B API spesifikasyonu.
