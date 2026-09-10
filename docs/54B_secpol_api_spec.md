# 54B - AppArmor/SELinux API Spec

## API
```c
int secpol64_init(void);
int secpol64_add_apparmor(const char *name, const char *path_prefix, int enforce);
int secpol64_add_rule(const char *profile, const char *path, u32 ops, int selinux, u32 *rule_id);
int secpol64_check(const char *profile, const char *path, u32 op);
int secpol64_set_mode(const char *profile, int enforce);
int secpol64_count(void);
int secpol64_profile_count(void);
```

## Donus Kurallari
- add_apparmor: NULL -1; yinelenen ad -2; dolu -3.
- add_rule: NULL -1; ops 0 -1; dolu -2; bilinmeyen profil -3.
- check: NULL/bilinmeyen -1; izin 1; red 0.
- set_mode: bilinmeyen profil -1.
