# 53B - Landlock API Spec

## API
```c
int landlk64_create_ruleset(u32 handled_access, int *out_id);
int landlk64_add_path_rule(int rs_id, const char *path, u32 allowed_access);
int landlk64_restrict_self(int rs_id);
int landlk64_check(const char *path, u32 access);
int landlk64_handled(int rs_id, u32 *out);
int landlk64_rule_count(int rs_id, int *out);
int landlk64_is_restricted(int rs_id, int *out);
```

## Donus Kurallari
- create: NULL -1; geçersiz handled -2; dolu -3.
- add_path_rule: bilinmeyen rs/NULL -1; restrict sonrası -2; handled dışı -3; dolu -4.
- check: NULL -1; izin 1; red 0.
