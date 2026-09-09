/* 49F: prometheus exporter — gosterge kaydi + metin bicimi. */
#include "arch/x86_64/longmode.h"

#define PROM64_MAX 32
#define PROM64_NAME 48
#define PROM64_HELP 96

struct prom64_metric {
    int used;
    char name[PROM64_NAME];
    char help[PROM64_HELP];
    u64 value;
};

static struct prom64_metric prom64_tab[PROM64_MAX];

static void prom_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct prom64_metric *prom64_find(const char *name) {
    int i, j;
    for (i = 0; i < PROM64_MAX; i++) {
        int same;
        if (!prom64_tab[i].used) continue;
        same = 1;
        for (j = 0; name[j] || prom64_tab[i].name[j]; j++) {
            if (name[j] != prom64_tab[i].name[j]) {
                same = 0;
                break;
            }
        }
        if (same) return &prom64_tab[i];
    }
    return 0;
}

int prom64_gauge(const char *name, const char *help, u64 value) {
    struct prom64_metric *m = prom64_find(name);
    int i;
    if (!name) return -1;
    if (m) {
        m->value = value;
        if (help) prom_str_copy(m->help, help, PROM64_HELP);
        return 0;
    }
    for (i = 0; i < PROM64_MAX; i++) {
        if (!prom64_tab[i].used) {
            prom64_tab[i].used = 1;
            prom_str_copy(prom64_tab[i].name, name, PROM64_NAME);
            prom_str_copy(prom64_tab[i].help, help ? help : "",
                          PROM64_HELP);
            prom64_tab[i].value = value;
            return 0;
        }
    }
    return -2;
}

int prom64_inc(const char *name) {
    struct prom64_metric *m = prom64_find(name);
    if (!m) return prom64_gauge(name, "", 1);
    m->value++;
    return 0;
}

static int prom64_u64_to_str(char *out, int max, u64 v) {
    char tmp[24];
    int n = 0, i, pos = 0;
    if (!v) {
        if (max < 2) return -1;
        out[0] = '0';
        out[1] = 0;
        return 1;
    }
    while (v && n < 24) {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    if (pos + n + 1 > max) return -1;
    for (i = n - 1; i >= 0; i--) out[pos++] = tmp[i];
    out[pos] = 0;
    return pos;
}

int prom64_render(char *out, int max) {
    int i, pos = 0;
    if (!out || max <= 0) return -1;
    out[0] = 0;
    for (i = 0; i < PROM64_MAX; i++) {
        struct prom64_metric *m = &prom64_tab[i];
        int j, r;
        const char *h1 = "# HELP ";
        const char *h2 = "# TYPE ";
        const char *g = " gauge\n";
        if (!m->used) continue;
        for (j = 0; h1[j] && pos + 1 < max; j++) out[pos++] = h1[j];
        for (j = 0; m->name[j] && pos + 1 < max; j++) out[pos++] = m->name[j];
        if (pos + 1 < max) out[pos++] = ' ';
        for (j = 0; m->help[j] && pos + 1 < max; j++) out[pos++] = m->help[j];
        if (pos + 1 < max) out[pos++] = '\n';
        for (j = 0; h2[j] && pos + 1 < max; j++) out[pos++] = h2[j];
        for (j = 0; m->name[j] && pos + 1 < max; j++) out[pos++] = m->name[j];
        for (j = 0; g[j] && pos + 1 < max; j++) out[pos++] = g[j];
        for (j = 0; m->name[j] && pos + 1 < max; j++) out[pos++] = m->name[j];
        if (pos + 1 < max) out[pos++] = ' ';
        r = prom64_u64_to_str(out + pos, max - pos, m->value);
        if (r < 0) return -2;
        pos += r;
        if (pos + 1 < max) out[pos++] = '\n';
    }
    if (pos < max)
        out[pos] = 0;
    else
        return -2;
    return pos;
}
