/* 42E: loginctl — oturum yasami: ac/etkinlestir/sonlandir/listele. */
#include "arch/x86_64/longmode.h"

#define LOGINCTL64_MAX 16

struct loginctl64_session {
    int used;
    int id;
    char user[32];
    char seat[32];
    int leader;
    int active;
};

static struct loginctl64_session loginctl64_tab[LOGINCTL64_MAX];
static int loginctl64_next = 1;
static int loginctl64_active_id = -1;

static void login_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int loginctl64_new_session(const char *user, const char *seat, int leader) {
    int i;
    if (!user) return -1;
    for (i = 0; i < LOGINCTL64_MAX; i++) {
        if (!loginctl64_tab[i].used) {
            loginctl64_tab[i].used = 1;
            loginctl64_tab[i].id = loginctl64_next++;
            login_str_copy(loginctl64_tab[i].user, user, 32);
            login_str_copy(loginctl64_tab[i].seat, seat ? seat : "", 32);
            loginctl64_tab[i].leader = leader;
            loginctl64_tab[i].active = 0;
            return loginctl64_tab[i].id;
        }
    }
    return -2;
}

int loginctl64_activate(int id) {
    int i;
    for (i = 0; i < LOGINCTL64_MAX; i++) {
        if (loginctl64_tab[i].used && loginctl64_tab[i].id == id) {
            int j;
            for (j = 0; j < LOGINCTL64_MAX; j++)
                if (loginctl64_tab[j].used) loginctl64_tab[j].active = 0;
            loginctl64_tab[i].active = 1;
            loginctl64_active_id = id;
            return 0;
        }
    }
    return -1;
}

int loginctl64_terminate(int id) {
    int i;
    for (i = 0; i < LOGINCTL64_MAX; i++) {
        if (loginctl64_tab[i].used && loginctl64_tab[i].id == id) {
            loginctl64_tab[i].used = 0;
            if (loginctl64_active_id == id) loginctl64_active_id = -1;
            return 0;
        }
    }
    return -1;
}

int loginctl64_list(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < LOGINCTL64_MAX && n < max; i++) {
        if (loginctl64_tab[i].used) ids[n++] = loginctl64_tab[i].id;
    }
    return n;
}

int loginctl64_active(int *id_out) {
    if (loginctl64_active_id < 0) return -1;
    if (id_out) *id_out = loginctl64_active_id;
    return 0;
}
