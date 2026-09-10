# 21A - Security Framework Tasarım

## Amaç
charOS için LSM-benzeri hook politikası: hook/subj/obj üçlüsü, first-match, default-deny.

## Kararlar
- **Hook**: 0=file_open,1=socket_create,2=ipc_send,3=proc_spawn. Aralık dışı reddedilir.
- **Politika**: 16 slot, monoton rule-id. Eşleşme hook+subj+obj tam eşitlik.
- **Karar**: allow eşleşmede 0, aksi halde -1 (default-deny).
- **Güvenlik**: NULL out, geçersiz hook/action reddedilir.

## Bileşenler
- `kernel/arch/x86_64/secfw64.c` (host-test davranış modeli)
- `include/arch/x86_64/secfw.h`
- Gerçek LSM hook (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
21B API spesifikasyonu.
