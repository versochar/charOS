# 41B - Package Manager API Spec

## API
```c
int pkgmgr64_init(void);
int pkgmgr64_install(const char *name, const char *version);
int pkgmgr64_remove(const char *name);
int pkgmgr64_query(const char *name, char *version, int max);
int pkgmgr64_upgrade_all(void);
int pkgmgr64_list_names(char *out[], int max);
int pkgmgr64_installed_count(void);
int pkgmgr64_state(int *out);
```

## Donus Kurallari
- 0 basarili; -1 argc hata; -2 mantiksal red (eski surum / kurulu degil / bulunamadi); -3 islem suruyor; -4 tablo dolu.
