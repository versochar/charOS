# 11I - Block Device Dokümantasyon

## Kullanıcı Kılavuzu
```c
blk64_init();
u64 d;
blk64_create(16, &d);
blk64_write(d, 0, 0xCAFE);
u64 v;
blk64_read(d, 0, &v);
```

## Durum
Tamamlandı.
