# 14I - Journaling Dokümantasyon

## Kullanıcı Kılavuzu
```c
jrnl64_init();
u64 tx;
jrnl64_begin(&tx);
jrnl64_append(tx, 0x1);
jrnl64_commit(tx);
u64 n;
jrnl64_replay(&n);
```

## Durum
Tamamlandı.
