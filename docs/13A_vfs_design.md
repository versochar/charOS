# 13A - Filesystems Core Tasarım

## Amaç
charOS için tek-değerli in-memory VFS çekirdeği: inode/fd ayrımı, güvenli open/close.

## Kararlar
- **Inode modeli**: 16 slot, monoton ino. Her inode tek u64 içerik tutar (v1).
- **FD modeli**: 32 slot, monoton fd. `open` inode→fd bağlar, `close` serbest bırakır.
- **Yaşam döngüsü**: `unlink` inode'u FREE yapar; açık fd varken unlink -2 (busy).
- **Güvenlik**: bilinmeyen ino/fd, NULL out reddedilir.

## Bileşenler
- `kernel/arch/x86_64/vfs64.c` (host-test davranış modeli)
- `include/arch/x86_64/vfs.h`
- Gerçek ext4/tmpfs (plan) rezerve.

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
13B API spesifikasyonu.
