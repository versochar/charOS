# 52B - Flatpak Sandbox API Spec

## API
```c
int fpsb64_init(void);
int fpsb64_create(const char *app_id, u32 perms, const char *runtime_ref, int *out_id);
int fpsb64_launch(int id);
int fpsb64_grant(int id, u32 bits);
int fpsb64_revoke(int id, u32 bits);
int fpsb64_has_perm(int id, u32 bits);
int fpsb64_state(int id, int *out);
int fpsb64_denied_count(int id, int *out);
int fpsb64_count(void);
```

## Donus Kurallari
- create: NULL -1; gecersiz bit -2; ayni app -3; dolu -4.
- launch: guvensiz NET+DEVICES -3 ve BLOCKED.
- grant/revoke: RUNNING iken -3.
