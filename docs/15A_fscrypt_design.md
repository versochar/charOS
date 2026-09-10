# 15A - Filesystem Encryption Tasarım

## Amaç
charOS için anahtarlı XOR davranış modeli: key-set zorunluluğu, wipe sonrası ret.

## Kararlar
- **Anahtar**: tek global u64, `setkey` ile kurulur, `wipe` ile sıfırlanır.
- **Şifreleme**: cipher = plain ^ key ^ GOLDEN. Key yoksa -2.
- **Wipe**: key=0 + has_key=0, sonrası encrypt/decrypt -2.
- **Güvenlik**: NULL out reddedilir. Gerçek AES-XTS (plan) rezerve.

## Bileşenler
- `kernel/arch/x86_64/fscrypt64.c` (host-test davranış modeli)
- `include/arch/x86_64/fscrypt.h`

## Başarı Kriterleri
Tasarım belgesi tamam, davranış modeli `make test-de64` ile yeşil.

## Sonraki Adım
15B API spesifikasyonu.
