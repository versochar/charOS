/* 57A: game launcher — oyun listesi + calistirma + favori + oynanma. */
#include "arch/x86_64/longmode.h"

#define GAMED64_MAX 64

struct gamed64_entry {
    int used;
    int hidden;
    char name[48];
    char exec[96];
    char icon[48];
    char genre[32];
    char cover[64];
    u64 playtime;
    u64 last_played;
    int fav;
};

static struct gamed64_entry gamed64_tab[GAMED64_MAX];

static void game_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
static int game_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int gamed64_add(const char *name, const char *exec, const char *icon, const char *genre) {
    int i;
    if (!name || !exec) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (!gamed64_tab[i].used) {
            gamed64_tab[i].used = 1;
            gamed64_tab[i].hidden = 0;
            game_str_copy(gamed64_tab[i].name, name, 48);
            game_str_copy(gamed64_tab[i].exec, exec, 96);
            game_str_copy(gamed64_tab[i].icon, icon ? icon : "", 48);
            game_str_copy(gamed64_tab[i].genre, genre ? genre : "", 32);
            game_str_copy(gamed64_tab[i].cover, "", 64);
            gamed64_tab[i].playtime = 0;
            gamed64_tab[i].last_played = 0;
            gamed64_tab[i].fav = 0;
            return 0;
        }
    }
    return -2;
}

int gamed64_query(const char *genre, char out[][48], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < GAMED64_MAX && n < max; i++) {
        if (!gamed64_tab[i].used || gamed64_tab[i].hidden) continue;
        if (genre && !game_str_eq(gamed64_tab[i].genre, genre)) continue;
        int j;
        for (j = 0; gamed64_tab[i].name[j] && j < 47; j++) out[n][j] = gamed64_tab[i].name[j];
        out[n][j] = 0;
        n++;
    }
    return n;
}

int gamed64_launch(const char *name, char *exec_out, int max) {
    int i, j;
    if (!name) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (!gamed64_tab[i].used) continue;
        if (!game_str_eq(gamed64_tab[i].name, name)) continue;
        if (exec_out && max > 0) {
            for (j = 0; gamed64_tab[i].exec[j] && j + 1 < max; j++) exec_out[j] = gamed64_tab[i].exec[j];
            exec_out[j] = 0;
        }
        gamed64_tab[i].last_played++;
        gamed64_tab[i].playtime += 1;
        return 0;
    }
    return -2;
}

int gamed64_fav(const char *name, int on) {
    int i;
    if (!name) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            gamed64_tab[i].fav = on ? 1 : 0;
            return 0;
        }
    }
    return -2;
}

int gamed64_remove(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            gamed64_tab[i].used = 0;
            return 0;
        }
    }
    return -2;
}

int gamed64_playtime(const char *name, u64 *out) {
    int i;
    if (!name || !out) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            *out = gamed64_tab[i].playtime;
            return 0;
        }
    }
    return -2;
}

int gamed64_set_playtime(const char *name, u64 mins) {
    int i;
    if (!name) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            gamed64_tab[i].playtime = mins;
            return 0;
        }
    }
    return -2;
}

int gamed64_lastplayed(const char *name, u64 *out) {
    int i;
    if (!name || !out) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            *out = gamed64_tab[i].last_played;
            return 0;
        }
    }
    return -2;
}

int gamed64_set_lastplayed(const char *name, u64 ts) {
    int i;
    if (!name) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && game_str_eq(gamed64_tab[i].name, name)) {
            gamed64_tab[i].last_played = ts;
            return 0;
        }
    }
    return -2;
}

int gamed64_top(int n, char out[][48], int max) {
    int idx[GAMED64_MAX], cnt = 0, i, j, k;
    if (n <= 0 || !out || max <= 0) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (gamed64_tab[i].used && !gamed64_tab[i].hidden) idx[cnt++] = i;
    }
    for (i = 0; i < cnt; i++) {
        for (j = i + 1; j < cnt; j++) {
            if (gamed64_tab[idx[j]].playtime > gamed64_tab[idx[i]].playtime) {
                int t = idx[i];
                idx[i] = idx[j];
                idx[j] = t;
            }
        }
    }
    if (n > cnt) n = cnt;
    if (n > max) n = max;
    for (i = 0; i < n; i++) {
        for (k = 0; gamed64_tab[idx[i]].name[k] && k < 47; k++) out[i][k] = gamed64_tab[idx[i]].name[k];
        out[i][k] = 0;
    }
    return n;
}

int gamed64_cover(const char *name, char *out, int max) {
    int i, k;
    if (!name || !out || max <= 0) return -1;
    for (i = 0; i < GAMED64_MAX; i++) {
        if (!gamed64_tab[i].used) continue;
        if (!game_str_eq(gamed64_tab[i].name, name)) continue;
        for (k = 0; gamed64_tab[i].cover[k] && k + 1 < max; k++) out[k] = gamed64_tab[i].cover[k];
        out[k] = 0;
        return 0;
    }
    return -2;
}
