# 15B - Encryption API Spesifikasyonu

## API
```c
int fscrypt64_init(void);
int fscrypt64_setkey(u64 key);
int fscrypt64_encrypt(u64 plain, u64 *out_cipher);
int fscrypt64_decrypt(u64 cipher, u64 *out_plain);
int fscrypt64_wipe(void);
```

## Sözleşme
- `encrypt/decrypt`: key yoksa -2, NULL out -1.
- `wipe`: her zaman 0, key'i sıfırlar.

## Sonraki Adım
15C implementasyon başlatma.
