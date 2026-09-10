# 46A - Partitioning Tools Design

## Amaç
GPT (GUID Partition Table) tablo araci: disk formatla, giriş ekle/sil, CRC korumasi, onarim.

## Kararlar
- **Dev başına**: total_lba + nparts + crc + corrupted bayragi.
- **Girisler**: name + type_code + first_lba..last_lba.
- Cakis tespiti, koruyucu bolge (LBA<2048, ESP haric) reddi.
- Bozuk tabloya yazim reddedilir; `repair` CRC'yi yeniden uretir.
- CRC: FNV-turevli (dev_id + girisler + total). Eklemek CRC'yi degistirir.

## Bilesenler
- `kernel/arch/x86_64/parttool64.c`, `include/arch/x86_64/parttool.h`

## Basari Kriterleri
- `make test-de64` 46A-J PASS.
