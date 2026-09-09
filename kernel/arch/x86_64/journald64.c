/* 42D: journald — yapilandirilmis halka gunluk + sorgu + budama. */
#include "arch/x86_64/longmode.h"

#define JOURNALD64_MAX 128
#define JOURNALD64_MSG 128
#define JOURNALD64_SVC 32

struct journald64_entry {
    u64 seq;
    int prio;
    char service[JOURNALD64_SVC];
    char msg[JOURNALD64_MSG];
};

static struct journald64_entry journald64_buf[JOURNALD64_MAX];
static int journald64_head = 0;
static int journald64_n = 0;
static u64 journald64_seq = 1;

static void journal_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int journal_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int journald64_write(int prio, const char *service, const char *msg) {
    struct journald64_entry *e = &journald64_buf[journald64_head];
    if (prio < 0 || prio > 7) return -1;
    e->seq = journald64_seq++;
    e->prio = prio;
    journal_str_copy(e->service, service ? service : "", JOURNALD64_SVC);
    journal_str_copy(e->msg, msg ? msg : "", JOURNALD64_MSG);
    journald64_head = (journald64_head + 1) % JOURNALD64_MAX;
    if (journald64_n < JOURNALD64_MAX) journald64_n++;
    return 0;
}

/* idx: servise ait kayitlar icinde 0=en eski. */
int journald64_query(const char *service, int idx, int *prio, char *msg,
                     int max) {
    int i, n = 0;
    if (!service || idx < 0) return -1;
    for (i = 0; i < journald64_n; i++) {
        int at = (journald64_head - journald64_n + i + JOURNALD64_MAX * 2) %
                 JOURNALD64_MAX;
        if (!journal_str_eq(journald64_buf[at].service, service)) continue;
        if (n++ != idx) continue;
        if (prio) *prio = journald64_buf[at].prio;
        if (msg && max > 0)
            journal_str_copy(msg, journald64_buf[at].msg, max);
        return 0;
    }
    return -2;
}

int journald64_count(const char *service) {
    int i, n = 0;
    if (!service) return 0;
    for (i = 0; i < journald64_n; i++) {
        int at = (journald64_head - journald64_n + i + JOURNALD64_MAX * 2) %
                 JOURNALD64_MAX;
        if (journal_str_eq(journald64_buf[at].service, service)) n++;
    }
    return n;
}

/* En fazla keep kayit tut (en yeniler); budanan sayi. */
int journald64_vacuum(int keep) {
    if (keep < 0) return -1;
    if (journald64_n <= keep) return 0;
    {
        int drop = journald64_n - keep;
        journald64_n = keep;
        return drop;
    }
}
