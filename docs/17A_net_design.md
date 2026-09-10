# 17A - Network Stack Core Tasarım

## Amaç
charOS için tek-paketli arayüz modeli: loopback/eth tipleri, tx/rx sayaçları.

## Kararlar
- **Arayüz modeli**: 8 slot, monoton if-id. Tip 0=loop,1=eth.
- **Paket**: arayüz başına 1 paketlik mailbox (u64) + valid flag.
- **Sayaç**: send tx++, recv rx++.
- **Güvenlik**: geçersiz iface, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/net64.c` (host-test davranış modeli)
- `include/arch/x86_64/net.h`
- Gerçek TCP/IP (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
17B API spesifikasyonu.
