/* 41C: dependency resolver — kisit cozumu + topolojik kurulum sirasi. */
#include "arch/x86_64/longmode.h"

#define DEPSOLVE64_MAXDEPS 16
#define DEPSOLVE64_MAXOUT 64

/* Bagimlilik bildirimi (testte statik tablo; gercek repo 41B meta ile). */
static struct {
    int used;
    char name[32];
    char ver[16];
    char deps[DEPSOLVE64_MAXDEPS][48];
    int ndeps;
} depsolve64_meta[64];

static int depsolve64_nmeta = 0;

static void dep_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int dep_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int depsolve64_add_meta(const char *name, const char *ver,
                        const char deps[][48], int ndeps);

/* "ad", "ad>=1.2", "ad=2.0", "ad<3.0", "ad<=2", "ad>1" */
int depsolve64_parse_dep(const char *s, struct depsolve64_dep *out) {
    int i = 0, j;
    if (!s || !out) return -1;
    while (s[i] && s[i] != '>' && s[i] != '<' && s[i] != '=' && i < 31) {
        out->name[i] = s[i];
        i++;
    }
    out->name[i] = 0;
    out->op = 0;
    out->ver[0] = 0;
    if (!s[i]) return 0; /* surumsuz */
    if (s[i] == '>' && s[i + 1] == '=') {
        out->op = 1;
        i += 2;
    } else if (s[i] == '<' && s[i + 1] == '=') {
        out->op = 2;
        i += 2;
    } else if (s[i] == '>') {
        out->op = 3;
        i++;
    } else if (s[i] == '<') {
        out->op = 4;
        i++;
    } else if (s[i] == '=') {
        out->op = 5;
        i++;
    }
    for (j = 0; s[i] && j < 15; j++, i++) out->ver[j] = s[i];
    out->ver[j] = 0;
    return 0;
}

static int dep_satisfies(const char *have, int op, const char *want) {
    int c;
    if (!op || !want[0]) return 1;
    c = repo64_vercmp(have, want);
    if (op == 1) return c >= 0;
    if (op == 2) return c <= 0;
    if (op == 3) return c > 0;
    if (op == 4) return c < 0;
    return c == 0; /* op 5: = */
}

/* Cozum: DFS, dongu tespiti, bagimlilik-once sirasi. */
static char solve_out[DEPSOLVE64_MAXOUT][32];
static int solve_n = 0;
static char solve_stack[DEPSOLVE64_MAXOUT][32];
static int solve_depth = 0;

static int solve_marked(const char *name) {
    int i;
    for (i = 0; i < solve_n; i++)
        if (dep_str_eq(solve_out[i], name)) return 1;
    return 0;
}

static int solve_in_stack(const char *name) {
    int i;
    for (i = 0; i < solve_depth; i++)
        if (dep_str_eq(solve_stack[i], name)) return 1;
    return 0;
}

static const char *solve_ver_of(const char *name) {
    int i;
    for (i = 0; i < depsolve64_nmeta; i++) {
        if (dep_str_eq(depsolve64_meta[i].name, name))
            return depsolve64_meta[i].ver;
    }
    return 0;
}

static int solve_visit(const char *name) {
    int i, m = -1;
    if (solve_marked(name)) return 0;
    if (solve_in_stack(name)) return -2; /* dongu */
    if (solve_depth >= DEPSOLVE64_MAXOUT) return -3;
    dep_str_copy(solve_stack[solve_depth++], name, 32);
    for (i = 0; i < depsolve64_nmeta; i++) {
        if (dep_str_eq(depsolve64_meta[i].name, name)) {
            m = i;
            break;
        }
    }
    if (m >= 0) {
        for (i = 0; i < depsolve64_meta[m].ndeps; i++) {
            struct depsolve64_dep d;
            const char *hv;
            int rc;
            if (depsolve64_parse_dep(depsolve64_meta[m].deps[i], &d) != 0) {
                solve_depth--;
                return -4;
            }
            hv = solve_ver_of(d.name);
            if (!hv) {
                /* Meta yok: repo en yuksegini kullan (41B) */
                static char rv[16];
                if (repo64_find(d.name, 0, rv, sizeof(rv)) != 0) {
                    solve_depth--;
                    return -5; /* bulunamadi */
                }
                hv = rv;
            }
            if (!dep_satisfies(hv, d.op, d.ver)) {
                solve_depth--;
                return -6; /* surum catismasi */
            }
            rc = solve_visit(d.name);
            if (rc != 0) {
                solve_depth--;
                return rc;
            }
        }
    }
    solve_depth--;
    if (solve_n >= DEPSOLVE64_MAXOUT) return -3;
    dep_str_copy(solve_out[solve_n++], name, 32);
    return 0;
}

int depsolve64_solve(const char *name, char out[][32], int max) {
    int rc, i;
    if (!name || !out || max <= 0) return -1;
    solve_n = 0;
    solve_depth = 0;
    rc = solve_visit(name);
    if (rc != 0) return rc;
    if (solve_n > max) return -7;
    for (i = 0; i < solve_n; i++) dep_str_copy(out[i], solve_out[i], 32);
    return solve_n;
}

int depsolve64_add_meta(const char *name, const char *ver,
                        const char deps[][48], int ndeps) {
    int i;
    if (!name || ndeps < 0 || ndeps > DEPSOLVE64_MAXDEPS) return -1;
    for (i = 0; i < depsolve64_nmeta; i++) {
        if (dep_str_eq(depsolve64_meta[i].name, name)) {
            dep_str_copy(depsolve64_meta[i].ver, ver ? ver : "", 16);
            depsolve64_meta[i].ndeps = 0;
            for (int j = 0; j < ndeps; j++) {
                dep_str_copy(depsolve64_meta[i].deps[j], deps[j], 48);
                depsolve64_meta[i].ndeps++;
            }
            return 0;
        }
    }
    if (depsolve64_nmeta >= 64) return -2;
    i = depsolve64_nmeta++;
    dep_str_copy(depsolve64_meta[i].name, name, 32);
    dep_str_copy(depsolve64_meta[i].ver, ver ? ver : "", 16);
    depsolve64_meta[i].ndeps = 0;
    for (int j = 0; j < ndeps; j++) {
        dep_str_copy(depsolve64_meta[i].deps[j], deps[j], 48);
        depsolve64_meta[i].ndeps++;
    }
    return 0;
}
