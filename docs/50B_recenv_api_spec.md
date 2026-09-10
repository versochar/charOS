# 50B - Recovery Environment API Spec

## API
```c
int recenv64_init(void);
int recenv64_boot_failed(void);
int recenv64_menu_timeout(int seconds);
int recenv64_enter(void);
int recenv64_select(int mode);
int recenv64_confirm(void);
int recenv64_countdown(int *seconds_left);
int recenv64_state(int *out);
int recenv64_selected_mode(int *out);
```

## Donus Kurallari
- menu_timeout: 0..300, disi -1. select: MENU degil -2, gecersiz mod -3.
- confirm: MENU degil -1. countdown: sure dolunca 1 ve RESOLVED.
