# 6I - Scheduling Dokümantasyon

## Kullanıcı Kılavuzu
```c
sched64_init();
sched64_add(pid_rt, 1);
sched64_add(pid_normal, 20);
u64 cur;
sched64_current(&cur); /* pid_rt beklenir */
sched64_yield();
sched64_tick();
```

## API Referansı
`sched64_init`, `sched64_add`, `sched64_remove`, `sched64_set_prio`, `sched64_current`, `sched64_yield`, `sched64_tick`.

## Not
Host-test davranış modelidir; gerçek preemption timer IRQ üzerinden yapılacaktır.

## Durum
Tamamlandı.
