/* 49H: logrotate — boyut esigi + kusak sayisi. */
#include "arch/x86_64/longmode.h"

#define LOGROTATE64_MAX 16

struct logrotate64_log {
    int used;
    char name[48];
    u64 size;
    u64 maxsize;
    int keep;
    int generations;
};

static struct logrotate64_log logrotate64_tab[LOGROTATE64_MAX];

static void log_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int log_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int logrotate64_add(const char *name, u64 maxsize, int keep) {
    int i;
    if (!name || !maxsize || keep < 0) return -1;
    for (i = 0; i < LOGROTATE64_MAX; i++) {
        if (logrotate64_tab[i].used &&
            log_str_eq(logrotate64_tab[i].name, name)) {
            logrotate64_tab[i].maxsize = maxsize;
            logrotate64_tab[i].keep = keep;
            return 0;
        }
    }
    for (i = 0; i < LOGROTATE64_MAX; i++) {
        if (!logrotate64_tab[i].used) {
            logrotate64_tab[i].used = 1;
            log_str_copy(logrotate64_tab[i].name, name, 48);
            logrotate64_tab[i].size = 0;
            logrotate64_tab[i].maxsize = maxsize;
            logrotate64_tab[i].keep = keep;
            logrotate64_tab[i].generations = 0;
            return 0;
        }
    }
    return -2;
}

int logrotate64_write(const char *name, u64 bytes) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LOGROTATE64_MAX; i++) {
        struct logrotate64_log *l = &logrotate64_tab[i];
        if (!l->used || !log_str_eq(l->name, name)) continue;
        l->size += bytes;
        while (l->size >= l->maxsize && l->generations < l->keep + 1) {
            l->size -= l->maxsize;
            l->generations++;
        }
        if (l->generations > l->keep + 1) l->generations = l->keep + 1;
        return 0;
    }
    return -2;
}

int logrotate64_generations(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LOGROTATE64_MAX; i++) {
        if (logrotate64_tab[i].used &&
            log_str_eq(logrotate64_tab[i].name, name))
            return logrotate64_tab[i].generations;
    }
    return -2;
}
