# 6B - Scheduling API Spesifikasyonu

## API
```c
int sched64_init(void);
int sched64_add(u64 pid, int prio);
int sched64_remove(u64 pid);
int sched64_set_prio(u64 pid, int prio);
int sched64_current(u64 *out_pid);
int sched64_yield(void);
int sched64_tick(void);
```

## Sözleşme
- `add`: pid==0 veya prio 0-31 dışında ise -1; zaten varsa -1; tablo doluysa -1.
- `remove`: bilinmeyen PID -1.
- `set_prio`: bilinmeyen PID veya aralık dışı prio -1.
- `current`: boş kuyrukta -1, aksi halde 0 + current pid.
- `yield/tick`: boş kuyrukta -1, aksi halde 0 ve rotasyon.

## Öncelik
Küçük prio değeri daha yüksek öncelik. RT 0-9, NORMAL 10-31.

## Uyumluluk
Mevcut process çekirdeği değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
6C implementasyon başlatma.
