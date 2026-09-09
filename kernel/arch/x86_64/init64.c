/* 42A: init cekirdegi — servis durum makinesi. */
#include "arch/x86_64/longmode.h"

#define INIT64_MAX 32

struct init64_svc {
    int used;
    char name[32];
    int state;
};

static struct init64_svc init64_tab[INIT64_MAX];

static void init_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int init_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct init64_svc *init64_find(const char *name) {
    int i;
    for (i = 0; i < INIT64_MAX; i++)
        if (init64_tab[i].used && init_str_eq(init64_tab[i].name, name))
            return &init64_tab[i];
    return 0;
}

int init64_add_service(const char *name) {
    int i;
    if (!name) return -1;
    if (init64_find(name)) return 0;
    for (i = 0; i < INIT64_MAX; i++) {
        if (!init64_tab[i].used) {
            init64_tab[i].used = 1;
            init_str_copy(init64_tab[i].name, name, 32);
            init64_tab[i].state = INIT64_STOPPED;
            return 0;
        }
    }
    return -2;
}

int init64_start(const char *name) {
    struct init64_svc *s = init64_find(name);
    if (!s) return -1;
    if (s->state == INIT64_RUNNING || s->state == INIT64_STARTING)
        return 0;
    s->state = INIT64_STARTING;
    /* Skeleton: baslatma aninda tamamlanir (gercek fork/exec 43x'te) */
    s->state = INIT64_RUNNING;
    return 0;
}

int init64_stop(const char *name) {
    struct init64_svc *s = init64_find(name);
    if (!s) return -1;
    if (s->state == INIT64_STOPPED) return 0;
    s->state = INIT64_STOPPING;
    s->state = INIT64_STOPPED;
    return 0;
}

int init64_state(const char *name) {
    struct init64_svc *s = init64_find(name);
    if (!s) return -1;
    return s->state;
}

int init64_count_state(int state) {
    int i, n = 0;
    for (i = 0; i < INIT64_MAX; i++)
        if (init64_tab[i].used && init64_tab[i].state == state) n++;
    return n;
}
