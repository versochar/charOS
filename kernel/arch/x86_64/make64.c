/* 54G: make — kural tablosu + DFS derleme + dongu tespiti. */
#include "arch/x86_64/longmode.h"

#define MAKE64_MAX_RULES 32
#define MAKE64_MAX_DEPS 8

struct make64_rule {
    int used;
    char target[48];
    char deps[MAKE64_MAX_DEPS][48];
    int ndeps;
    char recipe[96];
    u64 mtime;
};

static struct make64_rule make64_tab[MAKE64_MAX_RULES];

static void make_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int make_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int make64_add_rule(const char *target, const char *deps[],
                    const char *recipe) {
    int i;
    struct make64_rule *r = 0;
    if (!target) return -1;
    for (i = 0; i < MAKE64_MAX_RULES; i++) {
        if (make64_tab[i].used && make_str_eq(make64_tab[i].target, target)) {
            r = &make64_tab[i];
            break;
        }
    }
    if (!r) {
        for (i = 0; i < MAKE64_MAX_RULES; i++) {
            if (!make64_tab[i].used) {
                r = &make64_tab[i];
                r->used = 1;
                make_str_copy(r->target, target, 48);
                r->ndeps = 0;
                r->mtime = 0;
                break;
            }
        }
        if (!r) return -2;
    }
    r->ndeps = 0;
    if (deps) {
        for (i = 0; deps[i] && r->ndeps < MAKE64_MAX_DEPS; i++) {
            make_str_copy(r->deps[r->ndeps++], deps[i], 48);
        }
    }
    make_str_copy(r->recipe, recipe ? recipe : "", 96);
    return 0;
}

int make64_set_time(const char *target, u64 mtime) {
    int i;
    for (i = 0; i < MAKE64_MAX_RULES; i++) {
        if (make64_tab[i].used && make_str_eq(make64_tab[i].target, target)) {
            make64_tab[i].mtime = mtime;
            return 0;
        }
    }
    /* Kural yoksa dosya say: hayalet girdi */
    for (i = 0; i < MAKE64_MAX_RULES; i++) {
        if (!make64_tab[i].used) {
            make64_tab[i].used = 1;
            make_str_copy(make64_tab[i].target, target, 48);
            make64_tab[i].ndeps = 0;
            make64_tab[i].recipe[0] = 0;
            make64_tab[i].mtime = mtime;
            return 0;
        }
    }
    return -1;
}

static u64 make64_mtime_of(const char *target) {
    int i;
    for (i = 0; i < MAKE64_MAX_RULES; i++) {
        if (make64_tab[i].used && make_str_eq(make64_tab[i].target, target))
            return make64_tab[i].mtime;
    }
    return 0;
}

static char make64_built[MAKE64_MAX_RULES][48];
static int make64_nbuilt = 0;
static char make64_stack[MAKE64_MAX_RULES][48];
static int make64_depth = 0;

static int make64_marked(const char *t) {
    int i;
    for (i = 0; i < make64_nbuilt; i++) {
        if (make_str_eq(make64_built[i], t)) return 1;
    }
    return 0;
}

static int make64_in_stack(const char *t) {
    int i;
    for (i = 0; i < make64_depth; i++) {
        if (make_str_eq(make64_stack[i], t)) return 1;
    }
    return 0;
}

static struct make64_rule *make64_find(const char *t) {
    int i;
    for (i = 0; i < MAKE64_MAX_RULES; i++) {
        if (make64_tab[i].used && make_str_eq(make64_tab[i].target, t))
            return &make64_tab[i];
    }
    return 0;
}

static int make64_visit(const char *t) {
    struct make64_rule *r;
    int i, rc;
    u64 mmax = 0;
    if (make64_marked(t)) return 0;
    if (make64_in_stack(t)) return -2; /* dongu */
    if (make64_depth >= MAKE64_MAX_RULES) return -3;
    make_str_copy(make64_stack[make64_depth++], t, 48);
    r = make64_find(t);
    if (r && r->recipe[0]) {
        for (i = 0; i < r->ndeps; i++) {
            rc = make64_visit(r->deps[i]);
            if (rc != 0) {
                make64_depth--;
                return rc;
            }
            if (make64_mtime_of(r->deps[i]) > mmax)
                mmax = make64_mtime_of(r->deps[i]);
        }
        /* Eskiyse yeniden derle (zaman damgasi ilerler) */
        if (mmax >= r->mtime) {
            r->mtime = mmax + 1;
            if (make64_nbuilt < MAKE64_MAX_RULES)
                make_str_copy(make64_built[make64_nbuilt++], t, 48);
        }
    }
    make64_depth--;
    if (!make64_marked(t) && make64_nbuilt < MAKE64_MAX_RULES)
        make_str_copy(make64_built[make64_nbuilt++], t, 48);
    return 0;
}

int make64_build(const char *target, char built[][48], int max) {
    int rc, i;
    if (!target || !built || max <= 0) return -1;
    make64_nbuilt = 0;
    make64_depth = 0;
    rc = make64_visit(target);
    if (rc != 0) return rc;
    if (make64_nbuilt > max) return -4;
    for (i = 0; i < make64_nbuilt; i++)
        make_str_copy(built[i], make64_built[i], 48);
    return make64_nbuilt;
}
