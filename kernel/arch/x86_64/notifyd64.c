/* 56G: bildirim daemon — gonder/eylem/kapat/sure + liste. */
#include "arch/x86_64/longmode.h"

#define NOTIFYD64_MAX 32
#define NOTIFYD64_ACTIONS 4

struct notifyd64_entry {
    int used;
    int id;
    char app[48];
    char summary[64];
    char body[128];
    u64 expire_ms;
    int urgency; /* 0=dusuk, 1=normal, 2=kritik (suresiz) */
    char actions[NOTIFYD64_ACTIONS][32];
    int nactions;
    int responded;
};

static struct notifyd64_entry notifyd64_tab[NOTIFYD64_MAX];
static int notifyd64_next_id = 1;
static u64 notifyd64_now = 0;

static void notify_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int notify_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int notifyd64_send(const char *app, const char *summary, const char *body,
                   int timeout_ms) {
    int i;
    if (!app || !summary) return -1;
    for (i = 0; i < NOTIFYD64_MAX; i++) {
        if (!notifyd64_tab[i].used) {
            notifyd64_tab[i].used = 1;
            notifyd64_tab[i].id = notifyd64_next_id++;
            notify_str_copy(notifyd64_tab[i].app, app, 48);
            notify_str_copy(notifyd64_tab[i].summary, summary, 64);
            notify_str_copy(notifyd64_tab[i].body, body ? body : "", 128);
            notifyd64_tab[i].expire_ms =
                timeout_ms > 0 ? notifyd64_now + (u64)timeout_ms : 0;
            notifyd64_tab[i].urgency = 1;
            notifyd64_tab[i].nactions = 0;
            notifyd64_tab[i].responded = 0;
            return notifyd64_tab[i].id;
        }
    }
    return -2;
}

static struct notifyd64_entry *notifyd64_find(int id) {
    int i;
    for (i = 0; i < NOTIFYD64_MAX; i++) {
        if (notifyd64_tab[i].used && notifyd64_tab[i].id == id)
            return &notifyd64_tab[i];
    }
    return 0;
}

int notifyd64_action(int id, const char *key) {
    struct notifyd64_entry *e = notifyd64_find(id);
    int i;
    if (!e || !key) return -1;
    for (i = 0; i < e->nactions; i++) {
        if (notify_str_eq(e->actions[i], key)) {
            e->responded = 1;
            return 0;
        }
    }
    /* Kayitsiz eylem: varsayilan kapatma */
    e->responded = 1;
    e->used = 0;
    return 0;
}

int notifyd64_close(int id) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e) return -1;
    e->used = 0;
    return 0;
}

/* Suresi dolanlari temizler (kritik aciliyet suresizdir); kalan sayi.
 * simdi() testten beslenir. */
int notifyd64_expire(u64 now_ms) {
    int i, n = 0;
    notifyd64_now = now_ms;
    for (i = 0; i < NOTIFYD64_MAX; i++) {
        if (!notifyd64_tab[i].used) continue;
        if (notifyd64_tab[i].urgency != 2 && notifyd64_tab[i].expire_ms &&
            notifyd64_tab[i].expire_ms <= now_ms) {
            notifyd64_tab[i].used = 0;
            continue;
        }
        n++;
    }
    return n;
}

int notifyd64_list(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < NOTIFYD64_MAX && n < max; i++) {
        if (notifyd64_tab[i].used) ids[n++] = notifyd64_tab[i].id;
    }
    return n;
}

int notifyd64_add_action(int id, const char *key) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e || !key) return -1;
    if (e->nactions >= NOTIFYD64_ACTIONS) return -2;
    notify_str_copy(e->actions[e->nactions++], key, 32);
    return 0;
}

int notifyd64_set_urgency(int id, int level) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e || level < 0 || level > 2) return -1;
    e->urgency = level;
    return 0;
}

int notifyd64_urgency(int id) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e) return -1;
    return e->urgency;
}

int notifyd64_get(int id, char *app_out, int amax, char *sum_out, int smax,
                  char *body_out, int bmax) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e) return -1;
    if (app_out && amax > 0) notify_str_copy(app_out, e->app, amax);
    if (sum_out && smax > 0) notify_str_copy(sum_out, e->summary, smax);
    if (body_out && bmax > 0) notify_str_copy(body_out, e->body, bmax);
    return 0;
}

int notifyd64_replace(int id, const char *summary, const char *body) {
    struct notifyd64_entry *e = notifyd64_find(id);
    if (!e) return -1;
    if (summary) notify_str_copy(e->summary, summary, 64);
    if (body) notify_str_copy(e->body, body, 128);
    e->responded = 0;
    return 0;
}

int notifyd64_count(void) {
    int i, n = 0;
    for (i = 0; i < NOTIFYD64_MAX; i++) {
        if (notifyd64_tab[i].used) n++;
    }
    return n;
}

int notifyd64_clear_all(void) {
    int i, n = 0;
    for (i = 0; i < NOTIFYD64_MAX; i++) {
        if (notifyd64_tab[i].used) {
            notifyd64_tab[i].used = 0;
            n++;
        }
    }
    return n;
}
