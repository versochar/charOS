# 60B - Compliance Certifications API Spec

## API
```c
int comp64_init(void);
int comp64_cert_register(const char *name, int std, u32 version);
int comp64_cert_validate(int cert_id, char *out_hash, int max);
int comp64_cert_status(int cert_id, int *out_ok);
int comp64_report_list(char *buf, int max);
int comp64_report_export(const char *path);
int comp64_audit_log(int cert_id, const char *msg);
int comp64_compliance_check(int std, int *out_pass);
int comp64_cert_expire(int cert_id);
int comp64_cert_renew(int cert_id);
```
