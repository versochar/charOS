# 14B - Journaling API Spesifikasyonu

## API
```c
int jrnl64_init(void);
int jrnl64_begin(u64 *out_tx);
int jrnl64_append(u64 tx, u64 val);
int jrnl64_commit(u64 tx);
int jrnl64_abort(u64 tx);
int jrnl64_replay(u64 *out_count);
```

## Sözleşme
- `begin`: NULL out -1, tablo dolu -1.
- `append`: bilinmeyen tx -1, ACTIVE değilse -1, log doluysa -2.
- `commit/abort`: bilinmeyen tx -1, ACTIVE değilse -1.
- `replay`: NULL out -1, her zaman 0 döner + committed sayısı.

## Sonraki Adım
14C implementasyon başlatma.
