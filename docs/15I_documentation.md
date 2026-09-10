# 15I - Encryption Dokümantasyon

## Kullanıcı Kılavuzu
```c
fscrypt64_init();
fscrypt64_setkey(0x1234);
u64 c, p;
fscrypt64_encrypt(0xBEEF, &c);
fscrypt64_decrypt(c, &p);
fscrypt64_wipe();
```

## Durum
Tamamlandı.
