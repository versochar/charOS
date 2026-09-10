#include "arch/x86_64/sana.h"
#include <string.h>
#include <stdio.h>

struct sana_rule {
    sana64_rule_id_t id;
    int enabled;
    int severity;
    char desc[64];
};

static struct sana_rule rules[SANA64_MAX_RULES];
static int rule_cnt = 0;
static int total_issues = 0;
static int high_issues = 0;
static int initialized = 0;

int sana64_init(void) {
    memset(rules, 0, sizeof(rules));
    rule_cnt = 0;
    total_issues = 0;
    high_issues = 0;
    initialized = 1;
    return 0;
}

int sana64_register_rule(int id, int severity, const char *desc) {
    int i;
    if (!initialized) return -1;
    if (rule_cnt >= SANA64_MAX_RULES) return -2;
    if (!desc) return -3;
    rules[rule_cnt].id = (sana64_rule_id_t)id;
    rules[rule_cnt].enabled = 1;
    rules[rule_cnt].severity = severity;
    strncpy(rules[rule_cnt].desc, desc, sizeof(rules[rule_cnt].desc)-1);
    rules[rule_cnt].desc[sizeof(rules[rule_cnt].desc)-1] = '\0';
    rule_cnt++;
    return 0;
}

int sana64_rule_enabled(int id) {
    int i;
    if (!initialized) return 0;
    for (i = 0; i < rule_cnt; i++) {
        if (rules[i].id == (sana64_rule_id_t)id) return rules[i].enabled;
    }
    return 0;
}

int sana64_suppress_rule(int id) {
    int i;
    for (i = 0; i < rule_cnt; i++) {
        if (rules[i].id == (sana64_rule_id_t)id) { rules[i].enabled = 0; return 0; }
    }
    return -1;
}

int sana64_resume_rule(int id) {
    int i;
    for (i = 0; i < rule_cnt; i++) {
        if (rules[i].id == (sana64_rule_id_t)id) { rules[i].enabled = 1; return 0; }
    }
    return -1;
}

int sana64_analyze(const char *path, int *out_issues) {
    int i, found = 0;
    if (!initialized) return -1;
    if (!path || !out_issues) return -2;
    for (i = 0; i < rule_cnt; i++) {
        if (rules[i].enabled) {
            found++;
            if (rules[i].severity == SANA64_SEVERITY_HIGH) high_issues++;
        }
    }
    total_issues += found;
    *out_issues = found;
    return 0;
}

int sana64_report_issue(const char *path, int rule, u32 line,
                        char *buf, int max) {
    if (!path || !buf || max <= 0) return -1;
    if (max < 16) return -2;
    snprintf(buf, (size_t)max, "%s:%u:%d", path, (unsigned)line, (int)rule);
    return 0;
}

int sana64_summary(int *total, int *high) {
    if (!total || !high) return -1;
    *total = total_issues;
    *high = high_issues;
    return 0;
}

int sana64_configuration_check(void) {
    if (!initialized) return -1;
    if (rule_cnt == 0) return -2;
    return 0;
}

int sana64_scan_buffer(const u8 *buf, u16 len, int *out_find) {
    int i, cnt = 0;
    if (!buf || !out_find) return -1;
    if (len == 0) return -2;
    for (i = 0; i < (int)len; i++) {
        if (buf[i] == 0x00) cnt++;
        if (buf[i] == 0xFF) cnt++;
    }
    *out_find = cnt;
    return 0;
}