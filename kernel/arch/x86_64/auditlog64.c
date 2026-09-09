/* 38E: audit log — seri numarali halka tampon + tur aramasi. */
#include "arch/x86_64/longmode.h"

#define AUDITLOG64_MAX 128
#define AUDITLOG64_MSG 96

struct auditlog64_rec {
    u64 seq;
    int type;
    int pid;
    char msg[AUDITLOG64_MSG];
};

static struct auditlog64_rec auditlog64_buf[AUDITLOG64_MAX];
static int auditlog64_head = 0;
static int auditlog64_n = 0;
static u64 auditlog64_seq = 1;

static void audit_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int auditlog64_write(int type, int pid, const char *msg) {
    struct auditlog64_rec *r = &auditlog64_buf[auditlog64_head];
    r->seq = auditlog64_seq++;
    r->type = type;
    r->pid = pid;
    audit_str_copy(r->msg, msg ? msg : "", AUDITLOG64_MSG);
    auditlog64_head = (auditlog64_head + 1) % AUDITLOG64_MAX;
    if (auditlog64_n < AUDITLOG64_MAX) auditlog64_n++;
    return 0;
}

int auditlog64_read(u64 seq, int *type, int *pid, char *msg, int max) {
    int i;
    for (i = 0; i < auditlog64_n; i++) {
        int idx = (auditlog64_head - 1 - i + AUDITLOG64_MAX * 2) %
                  AUDITLOG64_MAX;
        if (auditlog64_buf[idx].seq == seq) {
            int j;
            if (type) *type = auditlog64_buf[idx].type;
            if (pid) *pid = auditlog64_buf[idx].pid;
            if (msg && max > 0) {
                for (j = 0; j + 1 < max && auditlog64_buf[idx].msg[j]; j++)
                    msg[j] = auditlog64_buf[idx].msg[j];
                msg[j] = 0;
            }
            return 0;
        }
    }
    return -1; /* yok / uzerine yazilmis */
}

u64 auditlog64_count_type(int type) {
    int i;
    u64 n = 0;
    for (i = 0; i < auditlog64_n; i++) {
        int idx = (auditlog64_head - 1 - i + AUDITLOG64_MAX * 2) %
                  AUDITLOG64_MAX;
        if (auditlog64_buf[idx].type == type) n++;
    }
    return n;
}

u64 auditlog64_next_seq(void) { return auditlog64_seq; }
