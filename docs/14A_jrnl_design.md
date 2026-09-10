# 14A - Filesystem Journaling Tasarım

## Amaç
charOS için write-ahead log davranış modeli: atomik begin/append/commit/abort, crash-replay sayacı.

## Kararlar
- **Tx modeli**: 16 slot, monoton tx-id. Durumlar FREE/ACTIVE/COMMITTED/ABORTED.
- **Append**: yalnızca ACTIVE tx'e, 8 giriş sınırında dolunca -2.
- **Commit**: ACTIVE→COMMITTED, replay sayacını artırır. Abort ACTIVE→ABORTED.
- **Replay**: COMMITTED sayısını döner ve sayaçı sıfırlar (v1).
- **Güvenlik**: bilinmeyen tx, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/jrnl64.c` (host-test davranış modeli)
- `include/arch/x86_64/jrnl.h`
- Gerçek FS journal (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
14B API spesifikasyonu.
