# 38B - Watchdog API Spec

## API
```c
int watchdog64_init(void);
int watchdog64_set_timeout(u8 ticks);
int watchdog64_pet(void); /* feed/pet */
int watchdog64_get_state(int* out_state); /* 0=OK,1=WARNING,2=RESET */
uint32_t watchdog64_get_ticks(void);
```

## Sözleşme
- `init`: her zaman 0.
- `set_timeout`: ticks>0 değilse -1.
- `pet`: init edilmedi ise -1.
- `get_state`: out NULL -1.
- `get_ticks`: timeout'a kalan tick (sayı).

## Sonraki Adım
38C implementasyon başlatma.
