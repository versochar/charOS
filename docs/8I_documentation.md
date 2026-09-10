# 8I - Synchronization Dokümantasyon

## Kullanıcı Kılavuzu
```c
sync64_init();
u64 sp, mt;
spin64_create(&sp);
spin64_trylock(sp);
spin64_unlock(sp);
mutex64_create(&mt);
mutex64_lock(mt, owner_pid);
mutex64_unlock(mt, owner_pid);
```

## API Referansı
`sync64_init`, `spin64_create/trylock/unlock/destroy`, `mutex64_create/lock/unlock/destroy`.

## Not
Host-test davranış modelidir; gerçek SMP atomikleri `core/spinlock.c` üzerinden yapılacaktır.

## Durum
Tamamlandı.
