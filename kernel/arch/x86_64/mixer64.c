/* 46C: mixer — kontrol tablosu + dB donusumu. */
#include "arch/x86_64/longmode.h"

#define MIXER64_MAX 32
#define MIXER64_MAX_CH 8

struct mixer64_ctl {
    int used;
    char name[32];
    int min;
    int max;
    int val[MIXER64_MAX_CH];
};

static struct mixer64_ctl mixer64_tab[MIXER64_MAX];

static void mixer_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int mixer_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct mixer64_ctl *mixer_find(const char *name) {
    int i;
    for (i = 0; i < MIXER64_MAX; i++)
        if (mixer64_tab[i].used && mixer_str_eq(mixer64_tab[i].name, name))
            return &mixer64_tab[i];
    return 0;
}

int mixer64_add(const char *name, int min, int max) {
    int i, c;
    if (!name || min > max) return -1;
    if (mixer_find(name)) return 0;
    for (i = 0; i < MIXER64_MAX; i++) {
        if (!mixer64_tab[i].used) {
            mixer64_tab[i].used = 1;
            mixer_str_copy(mixer64_tab[i].name, name, 32);
            mixer64_tab[i].min = min;
            mixer64_tab[i].max = max;
            for (c = 0; c < MIXER64_MAX_CH; c++)
                mixer64_tab[i].val[c] = max;
            return 0;
        }
    }
    return -2;
}

int mixer64_set(const char *name, int ch, int val) {
    struct mixer64_ctl *m = mixer_find(name);
    int c;
    if (!m) return -1;
    if (val < m->min) val = m->min;
    if (val > m->max) val = m->max;
    if (ch < 0) {
        for (c = 0; c < MIXER64_MAX_CH; c++) m->val[c] = val;
        return 0;
    }
    if (ch >= MIXER64_MAX_CH) return -2;
    m->val[ch] = val;
    return 0;
}

int mixer64_get(const char *name, int ch) {
    struct mixer64_ctl *m = mixer_find(name);
    if (!m || ch < 0 || ch >= MIXER64_MAX_CH) return -1000000;
    return m->val[ch];
}

/* dB (x100) <-> register (0..255): reg = (db+9600)/50 ornegi. */
int mixer64_db_to_reg(int db) {
    int r = (db + 9600) / 50;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    return r;
}

int mixer64_reg_to_db(int reg) {
    if (reg < 0) reg = 0;
    if (reg > 255) reg = 255;
    return reg * 50 - 9600;
}
