# 61B - Enterprise Features API Spec

## API
```c
int ent64_init(void);
int ent64_user_add(const char *name, int role_id);
int ent64_user_remove(const char *name);
int ent64_user_exists(const char *name);
int ent64_role_create(int role_id, const char *role_name);
int ent64_role_assign(int user_index, int role_id);
int ent64_policy_check(int user_index, int resource_id);
int ent64_audit_log(const char *msg);
int ent64_license_validate(const char *key, int *out_ok);
int ent64_feature_enable(int feature_id);
int ent64_feature_status(int feature_id);
int ent64_report_users(char *buf, int max);
```
