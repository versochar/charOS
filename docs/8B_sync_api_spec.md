# 8B - Synchronization API Spesifikasyonu

## API
```c
int sync64_init(void);
int spin64_create(u64 *out_id);
int spin64_trylock(u64 id);
int spin64_unlock(u64 id);
int spin64_destroy(u64 id);
int mutex64_create(u64 *out_id);
int mutex64_lock(u64 id, u64 owner);
int mutex64_unlock(u64 id, u64 owner);
int mutex64_destroy(u64 id);
```

## Sözleşme
- `create`: out NULL ise -1, tablo doluysa -1.
- `spin trylock`: bilinmeyen id -1, kilitliyse -1, başarıda 0.
- `spin unlock`: bilinmeyen id -1, kilitsizse -1.
- `mutex lock`: bilinmeyen id -1, kilitliyse -1 (bloklama yok v1).
- `mutex unlock`: bilinmeyen id -1, kilitsizse -1, owner uyuşmazsa -1.
- `destroy`: bilinmeyen id -1.

## Uyumluluk
Mevcut spinlock/futex çekirdeği değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
8C implementasyon başlatma.
