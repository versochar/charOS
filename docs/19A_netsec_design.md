# 19A - Network Security Tasarım

## Amaç
charOS için first-match firewall davranış modeli: allow/deny kuralları, default-deny.

## Kararlar
- **Kural modeli**: 16 slot, monoton rule-id. Her kural addr/port/action (0=deny,1=allow).
- **Eşleşme**: addr+port tam eşleşme, first-match kazanır. Eşleşme yoksa -1 (deny).
- **Yaşam döngüsü**: del slotu FREE yapar, sonrası -1.
- **Güvenlik**: NULL out, geçersiz action reddedilir.

## Bileşenler
- `kernel/arch/x86_64/netsec64.c` (host-test davranış modeli)
- `include/arch/x86_64/netsec.h`
- Gerçek netfilter/ebpf (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
19B API spesifikasyonu.
