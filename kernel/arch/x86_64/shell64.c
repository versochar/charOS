/* 54A: shell — jetonlama + degisken + yerlesikler + gecmis. */
#include "arch/x86_64/longmode.h"

#define SHELL64_MAX_TOKENS 16
#define SHELL64_HIST 32

static char shell64_hist[SHELL64_HIST][128];
static int shell64_hn = 0;

int shell64_tokenize(const char *line, char out[][64], int max) {
    int n = 0, i;
    const char *p;
    if (!line || !out || max <= 0) return -1;
    p = line;
    while (*p && n < max) {
        int len = 0, quote = 0;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '#') break; /* yorum */
        while (*p && len < 63) {
            if (quote) {
                if (*p == quote) {
                    quote = 0;
                    p++;
                    continue;
                }
                if (*p == '\\' && p[1]) p++;
                out[n][len++] = *p++;
                continue;
            }
            if (*p == '\'' || *p == '"') {
                quote = *p++;
                continue;
            }
            if (*p == '\\' && p[1]) {
                p++;
                out[n][len++] = *p++;
                continue;
            }
            if (*p == ' ' || *p == '\t' || *p == '#') break;
            out[n][len++] = *p++;
        }
        out[n][len] = 0;
        if (len) n++;
        if (*p == '#') break;
    }
    for (i = n; i < max; i++) out[i][0] = 0;
    return n;
}

/* $VAR ve ${VAR} genisletme (tek degisken). */
int shell64_expand(const char *in, const char *var, const char *val,
                   char *out, int max) {
    int o = 0, vlen = 0;
    if (!in || !var || !out || max <= 0) return -1;
    if (!val) val = "";
    while (var[vlen]) vlen++;
    while (*in && o + 1 < max) {
        if (in[0] == '$') {
            int k, match = 0;
            if (in[1] == '{') {
                for (k = 0; k < vlen && in[2 + k] == var[k]; k++) {
                }
                if (k == vlen && in[2 + k] == '}') {
                    int j;
                    for (j = 0; val[j] && o + 1 < max; j++)
                        out[o++] = val[j];
                    in += 3 + vlen;
                    match = 1;
                }
            } else {
                for (k = 0; k < vlen && in[1 + k] == var[k]; k++) {
                }
                if (k == vlen) {
                    int j;
                    for (j = 0; val[j] && o + 1 < max; j++)
                        out[o++] = val[j];
                    in += 1 + vlen;
                    match = 1;
                }
            }
            if (match) continue;
        }
        out[o++] = *in++;
    }
    out[o] = 0;
    return 0;
}

static int shell_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* Yerlesikler: echo/cd/pwd/exit/true/false. Donus cikis kodu. */
int shell64_builtin(const char *cmd, int argc, char argv[][64], char *out,
                    int max) {
    int i, o = 0;
    if (!cmd) return 127;
    if (shell_str_eq(cmd, "true")) return 0;
    if (shell_str_eq(cmd, "false")) return 1;
    if (shell_str_eq(cmd, "exit")) return 0;
    if (shell_str_eq(cmd, "pwd")) {
        const char *p = "/home/mitsune";
        if (!out || max <= 0) return 0;
        for (i = 0; p[i] && o + 1 < max; i++) out[o++] = p[i];
        out[o] = 0;
        return 0;
    }
    if (shell_str_eq(cmd, "cd")) {
        (void)argc;
        (void)argv;
        return 0; /* skeleton: her zaman basarili */
    }
    if (shell_str_eq(cmd, "echo")) {
        int nl = 1, start = 0;
        if (!out || max <= 0) return 0;
        if (argc > 0 && shell_str_eq(argv[0], "-n")) {
            nl = 0;
            start = 1;
        }
        for (i = start; i < argc; i++) {
            int j;
            for (j = 0; argv[i][j] && o + 1 < max; j++)
                out[o++] = argv[i][j];
            if (i + 1 < argc && o + 1 < max) out[o++] = ' ';
        }
        if (nl && o + 1 < max) out[o++] = '\n';
        out[o] = 0;
        return 0;
    }
    return 127; /* yerlesik degil */
}

int shell64_history_add(const char *line) {
    int i;
    if (!line) return -1;
    if (shell64_hn < SHELL64_HIST) {
        for (i = 0; line[i] && i < 127; i++)
            shell64_hist[shell64_hn][i] = line[i];
        shell64_hist[shell64_hn][i] = 0;
        shell64_hn++;
    } else {
        for (i = 0; i < SHELL64_HIST - 1; i++) {
            int j;
            for (j = 0; j < 128; j++) {
                shell64_hist[i][j] = shell64_hist[i + 1][j];
                if (!shell64_hist[i][j]) break;
            }
        }
        for (i = 0; line[i] && i < 127; i++)
            shell64_hist[SHELL64_HIST - 1][i] = line[i];
        shell64_hist[SHELL64_HIST - 1][i] = 0;
    }
    return 0;
}

int shell64_history_get(int idx, char *out, int max) {
    int i;
    if (idx < 0 || idx >= shell64_hn || !out || max <= 0) return -1;
    for (i = 0; shell64_hist[idx][i] && i + 1 < max; i++)
        out[i] = shell64_hist[idx][i];
    out[i] = 0;
    return 0;
}
