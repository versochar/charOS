/* 57B: controller mapping — kumanda ekleme + tus eşleme + profil. */
#include "arch/x86_64/longmode.h"

#define CTRL64_MAX 8
#define CTRL64_MAP_MAX 64
#define CTRL64_PROF_MAX 8

struct ctrl_map {
    int used;
    int button;
    int action;
};

struct ctrl_prof {
    int used;
    char name[48];
};

struct ctrl_entry {
    int used;
    int id;
    char name[48];
    int deadzone;
    int rumble_left;
    int rumble_right;
    struct ctrl_map maps[CTRL64_MAP_MAX];
    int map_cnt;
    struct ctrl_prof profs[CTRL64_PROF_MAX];
    int prof_cnt;
};

static struct ctrl_entry ctrl_tab[CTRL64_MAX];

static void ctrl_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
static int ctrl_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct ctrl_entry *ctrl_find(int id) {
    int i;
    for (i = 0; i < CTRL64_MAX; i++) {
        if (ctrl_tab[i].used && ctrl_tab[i].id == id) return &ctrl_tab[i];
    }
    return 0;
}

int controller64_add(int id, const char *name) {
    int i;
    if (id < 0 || !name) return -1;
    for (i = 0; i < CTRL64_MAX; i++) {
        if (ctrl_tab[i].used && ctrl_tab[i].id == id) return -2;
    }
    for (i = 0; i < CTRL64_MAX; i++) {
        if (!ctrl_tab[i].used) {
            ctrl_tab[i].used = 1;
            ctrl_tab[i].id = id;
            ctrl_str_copy(ctrl_tab[i].name, name, 48);
            ctrl_tab[i].deadzone = 15;
            ctrl_tab[i].rumble_left = 0;
            ctrl_tab[i].rumble_right = 0;
            ctrl_tab[i].map_cnt = 0;
            ctrl_tab[i].prof_cnt = 0;
            int j;
            for (j = 0; j < CTRL64_MAP_MAX; j++) ctrl_tab[i].maps[j].used = 0;
            for (j = 0; j < CTRL64_PROF_MAX; j++) ctrl_tab[i].profs[j].used = 0;
            return 0;
        }
    }
    return -3;
}

int controller64_map(int id, int button, int action) {
    struct ctrl_entry *c = ctrl_find(id);
    int i;
    if (!c) return -1;
    for (i = 0; i < CTRL64_MAP_MAX; i++) {
        if (c->maps[i].used && c->maps[i].button == button) {
            c->maps[i].action = action;
            return 0;
        }
    }
    for (i = 0; i < CTRL64_MAP_MAX; i++) {
        if (!c->maps[i].used) {
            c->maps[i].used = 1;
            c->maps[i].button = button;
            c->maps[i].action = action;
            c->map_cnt++;
            return 0;
        }
    }
    return -2;
}

int controller64_get_mapping(int id, int button, int *action_out) {
    struct ctrl_entry *c = ctrl_find(id);
    int i;
    if (!c || !action_out) return -1;
    for (i = 0; i < CTRL64_MAP_MAX; i++) {
        if (c->maps[i].used && c->maps[i].button == button) {
            *action_out = c->maps[i].action;
            return 0;
        }
    }
    return -2;
}

int controller64_remove_mapping(int id, int button) {
    struct ctrl_entry *c = ctrl_find(id);
    int i;
    if (!c) return -1;
    for (i = 0; i < CTRL64_MAP_MAX; i++) {
        if (c->maps[i].used && c->maps[i].button == button) {
            c->maps[i].used = 0;
            c->map_cnt--;
            return 0;
        }
    }
    return -2;
}

int controller64_deadzone(int id, int dz) {
    struct ctrl_entry *c = ctrl_find(id);
    if (!c) return -1;
    if (dz < 0 || dz > 100) return -2;
    c->deadzone = dz;
    return 0;
}

int controller64_rumble(int id, int left, int right) {
    struct ctrl_entry *c = ctrl_find(id);
    if (!c) return -1;
    if (left < 0 || left > 100 || right < 0 || right > 100) return -2;
    c->rumble_left = left;
    c->rumble_right = right;
    return 0;
}

int controller64_profile_create(int id, const char *name) {
    struct ctrl_entry *c = ctrl_find(id);
    int i;
    if (!c || !name) return -1;
    for (i = 0; i < CTRL64_PROF_MAX; i++) {
        if (c->profs[i].used && ctrl_str_eq(c->profs[i].name, name)) return -2;
    }
    for (i = 0; i < CTRL64_PROF_MAX; i++) {
        if (!c->profs[i].used) {
            c->profs[i].used = 1;
            ctrl_str_copy(c->profs[i].name, name, 48);
            c->prof_cnt++;
            return 0;
        }
    }
    return -3;
}

int controller64_profile_apply(int id, const char *name) {
    struct ctrl_entry *c = ctrl_find(id);
    int i;
    if (!c || !name) return -1;
    for (i = 0; i < CTRL64_PROF_MAX; i++) {
        if (c->profs[i].used && ctrl_str_eq(c->profs[i].name, name)) {
            /* profil uygulanmış sayılır */
            return 0;
        }
    }
    return -2;
}

int controller64_list(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < CTRL64_MAX && n < max; i++) {
        if (ctrl_tab[i].used) ids[n++] = ctrl_tab[i].id;
    }
    return n;
}
