# 18A - TCP/IP Tasarım

## Amaç
charOS için durum makinalı soket modeli: bind/connect sonrası tek-paketli send/recv.

## Kararlar
- **Soket modeli**: 8 slot, monoton sock-id. Durumlar FREE/CREATED/BOUND/CONNECTED.
- **Geçiş**: socket→CREATED, bind→BOUND, connect→CONNECTED. Sıra dışı geçiş -2.
- **Paket**: bağlı sokette 1 paketlik mailbox + valid flag.
- **Güvenlik**: bilinmeyen sock, NULL out, sırasız çağrı reddedilir.

## Bileşenler
- `kernel/arch/x86_64/tcp64.c` (host-test davranış modeli)
- `include/arch/x86_64/tcp.h`
- Gerçek TCP (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
18B API spesifikasyonu.
