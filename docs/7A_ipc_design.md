# 7A - IPC & Messaging Tasarım

## Amaç
charOS için hafif kanal tabanlı IPC: tek slot mailbox, bloklamasız send/recv, güvenli kanal yaşam döngüsü.

## Kararlar
- **Kanal modeli**: 64 slot, monoton chan-id. Her kanal 1 mesajlık mailbox (u64).
- **Semantik**: `send` doluysa -1, `recv` boşsa -1. Bloklama yok (v1).
- **Yaşam döngüsü**: `create` FREE slot, `destroy` slotu FREE yapar; destroy sonrası işlemler -1.
- **Güvenlik**: geçersiz chan reddedilir, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/ipc64.c` (host-test davranış modeli)
- `include/arch/x86_64/ipc.h`
- Gerçek zero-copy ring + futex wait (plan) rezerve; mevcut çekirdek bozulmaz.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
7B API spesifikasyonu.
