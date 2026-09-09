/* 42H: boot target — wants bagimliliklariyla topolojik baslatma plani. */
#include "arch/x86_64/longmode.h"

#define BOOTTARGET64_MAX 16
#define BOOTTARGET64_WANTS 8

struct boottarget64_entry {
    int used;
    char target[32];
    char wants[BOOTTARGET64_WANTS][32];
    int nwants;
};

static struct boottarget64_entry boottarget64_tab[BOOTTARGET64_MAX];
static char boottarget64_default[32];

static void target_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int target_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int boottarget64_add(const char *target, const char *wants) {
    int i;
    struct boottarget64_entry *e = 0;
    if (!target) return -1;
    for (i = 0; i < BOOTTARGET64_MAX; i++) {
        if (boottarget64_tab[i].used &&
            target_str_eq(boottarget64_tab[i].target, target)) {
            e = &boottarget64_tab[i];
            break;
        }
    }
    if (!e) {
        for (i = 0; i < BOOTTARGET64_MAX; i++) {
            if (!boottarget64_tab[i].used) {
                e = &boottarget64_tab[i];
                e->used = 1;
                target_str_copy(e->target, target, 32);
                e->nwants = 0;
                break;
            }
        }
        if (!e) return -2;
    }
    e->nwants = 0;
    if (wants && e) {
        const char *p = wants;
        while (*p && e->nwants < BOOTTARGET64_WANTS) {
            int k = 0;
            while (*p == ' ' || *p == '\t' || *p == ',') p++;
            if (!*p) break;
            while (*p && *p != ',' && *p != ' ' && *p != '\t' && k < 31)
                e->wants[e->nwants][k++] = *p++;
            e->wants[e->nwants][k] = 0;
            if (k) e->nwants++;
        }
    }
    return 0;
}

int boottarget64_set_default(const char *target) {
    if (!target) return -1;
    target_str_copy(boottarget64_default, target, 32);
    return 0;
}

static char plan_out[BOOTTARGET64_MAX][32];
static int plan_n = 0;
static char plan_stack[BOOTTARGET64_MAX][32];
static int plan_depth = 0;

static int plan_marked(const char *t) {
    int i;
    for (i = 0; i < plan_n; i++)
        if (target_str_eq(plan_out[i], t)) return 1;
    return 0;
}

static int plan_in_stack(const char *t) {
    int i;
    for (i = 0; i < plan_depth; i++)
        if (target_str_eq(plan_stack[i], t)) return 1;
    return 0;
}

static struct boottarget64_entry *plan_find(const char *t) {
    int i;
    for (i = 0; i < BOOTTARGET64_MAX; i++)
        if (boottarget64_tab[i].used &&
            target_str_eq(boottarget64_tab[i].target, t))
            return &boottarget64_tab[i];
    return 0;
}

static int plan_visit(const char *t) {
    struct boottarget64_entry *e;
    int i, rc;
    if (plan_marked(t)) return 0;
    if (plan_in_stack(t)) return -2;
    if (plan_depth >= BOOTTARGET64_MAX) return -3;
    target_str_copy(plan_stack[plan_depth++], t, 32);
    e = plan_find(t);
    if (e) {
        for (i = 0; i < e->nwants; i++) {
            rc = plan_visit(e->wants[i]);
            if (rc != 0) {
                plan_depth--;
                return rc;
            }
        }
    }
    plan_depth--;
    if (plan_n >= BOOTTARGET64_MAX) return -3;
    target_str_copy(plan_out[plan_n++], t, 32);
    return 0;
}

int boottarget64_plan(const char *target, char out[][32], int max) {
    const char *t = target ? target : boottarget64_default;
    int rc, i;
    if (!t || !t[0] || !out || max <= 0) return -1;
    plan_n = 0;
    plan_depth = 0;
    rc = plan_visit(t);
    if (rc != 0) return rc;
    if (plan_n > max) return -4;
    for (i = 0; i < plan_n; i++) target_str_copy(out[i], plan_out[i], 32);
    return plan_n;
}
