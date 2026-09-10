# 56B - Hardening & Mitigations API Spec

## API
```c
int mitig64_init(void);
int mitig64_vuln_apply(int vuln_id, u32 tech_bits);
int mitig64_vuln_status(int vuln_id, int *out_status);
int mitig64_mitigated_count(void);
int mitig64_unmitigated_count(void);
int mitig64_auto_verify(u32 reported_tech_bits);
int mitig64_cpu_trustworthy(void);
int mitig64_report(u32 tech_mask, char *buf, int max);
```

## Donus Kurallari
- vuln_apply: gecersiz id -1; tech 0 -2; basarisiz uygulama yok.
- vuln_status: gecersiz id / null cikti -1.
- report: null/kucuk buf -1, kucuk uzunluk -2.
