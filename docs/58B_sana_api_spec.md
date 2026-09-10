# 58B - Static Analysis API Spec

## API
```c
int sana64_init(void);
int sana64_register_rule(int id, int severity, const char *desc);
int sana64_rule_enabled(int id);
int sana64_analyze(const char *path, int *out_issues);
int sana64_report_issue(const char *path, int rule, u32 line, char *buf, int max);
int sana64_summary(int *total, int *high);
int sana64_configuration_check(void);
int sana64_suppress_rule(int id);
int sana64_resume_rule(int id);
int sana64_scan_buffer(const u8 *buf, u16 len, int *out_find);
```
