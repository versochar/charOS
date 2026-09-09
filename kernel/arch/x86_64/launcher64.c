/* 56F: baslat menusu — .desktop girdileri + kategori + favori. */
#include "arch/x86_64/longmode.h"

#define LAUNCHER64_MAX 64

struct launcher64_entry {
    int used;
    int hidden;
    char name[48];
    char exec[96];
    char icon[48];
    char cats[64];
    int fav;
    u64 runs;
};

static struct launcher64_entry launcher64_tab[LAUNCHER64_MAX];

static void launcher_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int launcher_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* Kategori listesinde (noktali-virgullu) gecer mi? */
static int launcher_has_cat(const char *cats, const char *cat) {
    int i = 0;
    if (!cats || !cat) return 0;
    while (cats[i]) {
        int j = 0, k = i;
        while (cats[k] && cats[k] != ';' && cat[j] && cats[k] == cat[j]) {
            k++;
            j++;
        }
        if (!cat[j] && (cats[k] == ';' || !cats[k])) return 1;
        while (cats[i] && cats[i] != ';') i++;
        if (cats[i] == ';') i++;
    }
    return 0;
}

int launcher64_add(const char *name, const char *exec, const char *icon,
                   const char *cats) {
    int i;
    if (!name || !exec) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            launcher_str_copy(launcher64_tab[i].exec, exec, 96);
            if (icon) launcher_str_copy(launcher64_tab[i].icon, icon, 48);
            if (cats) launcher_str_copy(launcher64_tab[i].cats, cats, 64);
            return 0;
        }
    }
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (!launcher64_tab[i].used) {
            launcher64_tab[i].used = 1;
            launcher64_tab[i].hidden = 0;
            launcher_str_copy(launcher64_tab[i].name, name, 48);
            launcher_str_copy(launcher64_tab[i].exec, exec, 96);
            launcher_str_copy(launcher64_tab[i].icon, icon ? icon : "", 48);
            launcher_str_copy(launcher64_tab[i].cats, cats ? cats : "", 64);
            launcher64_tab[i].fav = 0;
            launcher64_tab[i].runs = 0;
            return 0;
        }
    }
    return -2;
}

int launcher64_query(const char *cat, char out[][48], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < LAUNCHER64_MAX && n < max; i++) {
        int j;
        if (!launcher64_tab[i].used) continue;
        if (launcher64_tab[i].hidden) continue;
        if (cat && !launcher_has_cat(launcher64_tab[i].cats, cat))
            continue;
        for (j = 0; launcher64_tab[i].name[j] && j < 47; j++)
            out[n][j] = launcher64_tab[i].name[j];
        out[n][j] = 0;
        n++;
    }
    return n;
}

int launcher64_launch(const char *name, char *exec_out, int max) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        int j;
        if (!launcher64_tab[i].used) continue;
        if (!launcher_str_eq(launcher64_tab[i].name, name)) continue;
        if (exec_out && max > 0) {
            for (j = 0; launcher64_tab[i].exec[j] && j + 1 < max; j++)
                exec_out[j] = launcher64_tab[i].exec[j];
            exec_out[j] = 0;
        }
        launcher64_tab[i].runs++;
        return 0;
    }
    return -2;
}

int launcher64_fav(const char *name, int on) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            launcher64_tab[i].fav = on ? 1 : 0;
            return 0;
        }
    }
    return -2;
}

int launcher64_remove(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            launcher64_tab[i].used = 0;
            return 0;
        }
    }
    return -2;
}

int launcher64_runs(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name))
            return launcher64_tab[i].runs > 0x7FFFFFFFULL
                       ? 0x7FFFFFFF
                       : (int)launcher64_tab[i].runs;
    }
    return -2;
}

/* En cok calistirilan n girdi (azalan). Donus adet. */
int launcher64_top(int n, char out[][48], int max) {
    int idx[LAUNCHER64_MAX], cnt = 0, i, j, k;
    if (n <= 0 || !out || max <= 0) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used && !launcher64_tab[i].hidden)
            idx[cnt++] = i;
    }
    for (i = 0; i < cnt; i++) {
        for (j = i + 1; j < cnt; j++) {
            if (launcher64_tab[idx[j]].runs > launcher64_tab[idx[i]].runs) {
                int t = idx[i];
                idx[i] = idx[j];
                idx[j] = t;
            }
        }
    }
    if (n > cnt) n = cnt;
    if (n > max) n = max;
    for (i = 0; i < n; i++) {
        for (k = 0; launcher64_tab[idx[i]].name[k] && k < 47; k++)
            out[i][k] = launcher64_tab[idx[i]].name[k];
        out[i][k] = 0;
    }
    return n;
}

/* Exec alanindaki %f/%F (dosya), %u/%U (url), %% degistirme. */
int launcher64_launch_argv(const char *name, const char *arg,
                           char *exec_out, int max) {
    int i, o = 0;
    const char *exec = 0;
    if (!name || !exec_out || max <= 0) return -1;
    if (!arg) arg = "";
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            exec = launcher64_tab[i].exec;
            break;
        }
    }
    if (!exec) return -2;
    while (*exec && o + 1 < max) {
        if (exec[0] == '%' && (exec[1] == 'f' || exec[1] == 'F' ||
                               exec[1] == 'u' || exec[1] == 'U')) {
            int k;
            for (k = 0; arg[k] && o + 1 < max; k++) exec_out[o++] = arg[k];
            exec += 2;
            continue;
        }
        if (exec[0] == '%' && exec[1] == '%') {
            exec_out[o++] = '%';
            exec += 2;
            continue;
        }
        exec_out[o++] = *exec++;
    }
    exec_out[o] = 0;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            launcher64_tab[i].runs++;
            break;
        }
    }
    return 0;
}

int launcher64_hide(const char *name, int on) {
    int i;
    if (!name) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (launcher64_tab[i].used &&
            launcher_str_eq(launcher64_tab[i].name, name)) {
            launcher64_tab[i].hidden = on ? 1 : 0;
            return 0;
        }
    }
    return -2;
}

int launcher64_icon(const char *name, char *out, int max) {
    int i, k;
    if (!name || !out || max <= 0) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        if (!launcher64_tab[i].used) continue;
        if (!launcher_str_eq(launcher64_tab[i].name, name)) continue;
        for (k = 0; launcher64_tab[i].icon[k] && k + 1 < max; k++)
            out[k] = launcher64_tab[i].icon[k];
        out[k] = 0;
        return 0;
    }
    return -2;
}

/* Ayirt-edici kategori listesi. Donus adet. */
int launcher64_cats(char out[][32], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < LAUNCHER64_MAX; i++) {
        const char *p;
        if (!launcher64_tab[i].used) continue;
        p = launcher64_tab[i].cats;
        while (*p && n < max) {
            char cat[32];
            int k = 0, dup = 0, m;
            while (*p == ';') p++;
            if (!*p) break;
            while (*p && *p != ';' && k < 31) cat[k++] = *p++;
            cat[k] = 0;
            if (!k) continue;
            for (m = 0; m < n; m++) {
                if (launcher_str_eq(out[m], cat)) {
                    dup = 1;
                    break;
                }
            }
            if (dup) continue;
            for (k = 0; cat[k] && k < 31; k++) out[n][k] = cat[k];
            out[n][k] = 0;
            n++;
        }
    }
    return n;
}
