#include "arch/x86_64/comp.h"
#include <string.h>
#include <stdio.h>

struct comp_cert {
    char name[COMP64_NAME_MAX];
    int std;
    u32 version;
    int valid;
    int expired;
    char hash[COMP64_HASH_MAX];
};

static struct comp_cert certs[COMP64_MAX_CERTS];
static int cert_cnt = 0;
static int initialized = 0;

int comp64_init(void) {
    memset(certs, 0, sizeof(certs));
    cert_cnt = 0;
    initialized = 1;
    return 0;
}

int comp64_cert_register(const char *name, int std, u32 version) {
    if (!initialized) return -1;
    if (!name) return -2;
    if (cert_cnt >= COMP64_MAX_CERTS) return -3;
    strncpy(certs[cert_cnt].name, name, COMP64_NAME_MAX-1);
    certs[cert_cnt].name[COMP64_NAME_MAX-1] = '\0';
    certs[cert_cnt].std = (int)std;
    certs[cert_cnt].version = version;
    certs[cert_cnt].valid = 1;
    certs[cert_cnt].expired = 0;
    snprintf(certs[cert_cnt].hash, COMP64_HASH_MAX, "sha256:%08x", version);
    cert_cnt++;
    return cert_cnt - 1;
}

int comp64_cert_validate(int cert_id, char *out_hash, int max) {
    if (!initialized) return -1;
    if (!out_hash || max <= 0) return -3;
    if (cert_id < 0 || cert_id >= cert_cnt) return -2;
    if (max < (int)sizeof(certs[cert_id].hash)) return -4;
    strcpy(out_hash, certs[cert_id].hash);
    return 0;
}

int comp64_cert_status(int cert_id, int *out_ok) {
    if (!initialized) return -1;
    if (cert_id < 0 || cert_id >= cert_cnt) return -2;
    if (!out_ok) return -3;
    *out_ok = certs[cert_id].valid && !certs[cert_id].expired;
    return 0;
}

int comp64_report_list(char *buf, int max) {
    int i;
    if (!buf || max <= 0) return -1;
    if (max < 32) return -2;
    buf[0] = '\0';
    for (i = 0; i < cert_cnt; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%s:%d:%d\n", certs[i].name, certs[i].std, certs[i].valid);
        if ((int)strlen(buf) + (int)strlen(line) >= max) break;
        strcat(buf, line);
    }
    return 0;
}

int comp64_report_export(const char *path) {
    (void)path;
    return -2;
}

int comp64_audit_log(int cert_id, const char *msg) {
    if (cert_id < 0 || cert_id >= cert_cnt) return -1;
    (void)msg;
    return 0;
}

int comp64_compliance_check(int std, int *out_pass) {
    int i, ok = 1;
    if (!out_pass) return -1;
    for (i = 0; i < cert_cnt; i++) {
        if (certs[i].std == (int)std) {
            if (!certs[i].valid || certs[i].expired) { ok = 0; break; }
        }
    }
    *out_pass = ok;
    return 0;
}

int comp64_cert_expire(int cert_id) {
    if (cert_id < 0 || cert_id >= cert_cnt) return -1;
    certs[cert_id].expired = 1;
    return 0;
}

int comp64_cert_renew(int cert_id) {
    if (cert_id < 0 || cert_id >= cert_cnt) return -1;
    certs[cert_id].expired = 0;
    certs[cert_id].valid = 1;
    return 0;
}