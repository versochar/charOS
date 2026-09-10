# 8A - Synchronization Primitives Tasarım

## Amaç
charOS için tek CPU davranış modelli senkronizasyon: spinlock (busy-check) + sahipli mutex, deadlock-önleyici kurallar.

## Kararlar
- **Spinlock**: tek slot kilit; `trylock` boşsa alır, doluysa -1. IRQ-koruması model dışı (not).
- **Mutex**: sahiplik takibi; kilidi alan owner bırakmalı, başkası bırakamaz. Recursive lock yok (v1).
- **ID yönetimi**: 64 slot, monoton id. Destroy sonrası kullanım -1.
- **Sıralama**: v1'de lock-ordering kontrolü yok, dokümante edildi.

## Bileşenler
- `kernel/arch/x86_64/sync64.c` (host-test davranış modeli)
- `include/arch/x86_64/sync.h`
- Gerçek SMP spinlock (`lock cmpxchg`) + futex wait (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
8B API spesifikasyonu.
