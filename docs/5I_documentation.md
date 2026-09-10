# 5I - Process Dokümantasyon

## Kullanıcı Kılavuzu
```c
proc64_init();
u64 pid;
proc64_spawn(&pid);
proc64_exit(pid, 0);
int code;
proc64_wait(pid, &code);
```

## API Referansı
`proc64_init`, `proc64_spawn`, `proc64_exit`, `proc64_wait`, `proc64_state`.

## Not
Host-test davranış modelidir; gerçek scheduling `process/` üzerinden yapılacaktır.

## Durum
Tamamlandı.
