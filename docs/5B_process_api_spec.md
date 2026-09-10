# 5B - Process API Spesifikasyonu

## API
```c
int proc64_init(void);
int proc64_spawn(u64 *out_pid);
int proc64_exit(u64 pid, int code);
int proc64_wait(u64 pid, int *out_code);
int proc64_state(u64 pid, int *out_state); /* 0=FREE,1=READY,2=RUNNING,3=ZOMBIE */
```

## Sözleşme
- `spawn`: out_pid NULL ise -1, başarıda 0 ve yeni PID.
- `exit`: bilinmeyen PID -1, zaten ZOMBIE/FREE ise -1.
- `wait`: yalnızca ZOMBIE'de 0, diğer durumda -1; başarılı wait slotu FREE yapar.
- `state`: bilinmeyen PID -1.

## Durum Kodları
0 FREE, 1 READY, 2 RUNNING, 3 ZOMBIE.

## Uyumluluk
Mevcut process çekirdeği değişmez; yeni API opsiyonel uzantıdır.

## Sonraki Adım
5C implementasyon başlatma.
