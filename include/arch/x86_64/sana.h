#ifndef SANA64_H
#define SANA64_H
#include "arch/x86_64/longmode.h"

#define SANA64_MAX_RULES  32
#define SANA64_MAX_PATH   256
#define SANA64_SEVERITY_LOW    0
#define SANA64_SEVERITY_MED    1
#define SANA64_SEVERITY_HIGH   2

typedef enum {
    SANA64_R_NULL_DEREF = 0,
    SANA64_R_BUF_OVERFLOW,
    SANA64_R_UNINIT_USE,
    SANA64_R_USE_AFTER_FREE,
    SANA64_R_INT_OVERFLOW,
    SANA64_R_DOUBLE_FREE,
    SANA64_R_PATH_TRAVERSAL,
    SANA64_R_HARDCODED_SECRET,
    SANA64_R_DEAD_CODE,
    SANA64_R_UNUSED_VAR
} sana64_rule_id_t;

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

#endif