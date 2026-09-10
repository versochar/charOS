#ifndef COMP64_H
#define COMP64_H
#include "arch/x86_64/longmode.h"

#define COMP64_MAX_CERTS  32
#define COMP64_NAME_MAX   64
#define COMP64_HASH_MAX   64

typedef enum {
    COMP64_STD_FIPS = 0,
    COMP64_STD_COMMON_CRITERIA,
    COMP64_STD_ISO27001,
    COMP64_STD_SOC2,
    COMP64_STD_GDPR,
    COMP64_STD_PCI_DSS
} comp64_std_t;

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

#endif