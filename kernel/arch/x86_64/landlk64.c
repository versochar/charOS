#include "arch/x86_64/landlk.h"
#include <string.h>

/* Landlock benzeri patika tabanli LSM.
 * Kurallar: (kurallar-kumesi, patika, izin bitleri). restrict_self sonrasi
 * tablo salt-okunur olur ve yeni kural eklenemez. */

struct landlk_rule {
    int  rs_id;
    char path[LANDLK64_PATH_MAX];
    u32  access;
    int  used;
};

struct landlk_rs {
    u32  handled;
    int  restricted;
    int  used;
    int  id;
};

static struct landlk_rs  rs_tab[LANDLK64_MAX_RS];
static struct landlk_rule rule_tab[LANDLK64_MAX_RULES];
static int rs_next = 1;
static int landlk_active = 0;
static int landlk_active_rs = -1;

static int landlk_rs_find(int rs_id) {
    int i;
    for (i = 0; i < LANDLK64_MAX_RS; i++)
        if (rs_tab[i].used && rs_tab[i].id == rs_id)
            return i;
    return -1;
}

static int landlk_has_prefix(const char *prefix, const char *path) {
    size_t n = strlen(prefix);
    if (strncmp(prefix, path, n) != 0) return 0;
    if (n > 0 && prefix[n - 1] == '/') return 1; /* alt agaci kucaklar */
    return path[n] == '\0' || path[n] == '/';
}

int landlk64_create_ruleset(u32 handled_access, int *out_id) {
    int i;
    if (!out_id) return -1;
    if (handled_access & ~LANDLK_ACCESS_ALL) return -2;
    for (i = 0; i < LANDLK64_MAX_RS; i++) {
        if (!rs_tab[i].used) {
            rs_tab[i].used = 1;
            rs_tab[i].id = rs_next++;
            rs_tab[i].handled = handled_access;
            rs_tab[i].restricted = 0;
            *out_id = rs_tab[i].id;
            return 0;
        }
    }
    return -3;
}

int landlk64_add_path_rule(int rs_id, const char *path, u32 allowed_access) {
    int i, r = landlk_rs_find(rs_id);
    if (r < 0 || !path) return -1;
    if (rs_tab[r].restricted) return -2; /* salt-okunur */
    if (allowed_access & ~rs_tab[r].handled) return -3;
    if (!path[0]) return -1;
    for (i = 0; i < LANDLK64_MAX_RULES; i++) {
        if (!rule_tab[i].used) {
            rule_tab[i].rs_id = rs_id;
            strncpy(rule_tab[i].path, path, LANDLK64_PATH_MAX - 1);
            rule_tab[i].access = allowed_access;
            rule_tab[i].used = 1;
            return 0;
        }
    }
    return -4;
}

int landlk64_restrict_self(int rs_id) {
    if (landlk_rs_find(rs_id) < 0) return -1;
    landlk_active_rs = rs_id;
    landlk_active = 1;
    rs_tab[landlk_rs_find(rs_id)].restricted = 1;
    return 0;
}

int landlk64_check(const char *path, u32 access) {
    struct landlk_rs *rs;
    int r, k, granted = 0;
    if (!path) return -1;
    if (!landlk_active || landlk_active_rs < 0) return 1;
    r = landlk_rs_find(landlk_active_rs);
    if (r < 0) return 1;
    rs = &rs_tab[r];
    for (k = 0; k < LANDLK64_MAX_RULES; k++) {
        if (rule_tab[k].used && rule_tab[k].rs_id == rs->id &&
            landlk_has_prefix(rule_tab[k].path, path) &&
            (rule_tab[k].access & access) == access) {
            granted = 1;
            break;
        }
    }
    if (granted) return 1;
    return 0; /* kuralla aciklanmayan erisim red */
}

int landlk64_handled(int rs_id, u32 *out) {
    int r = landlk_rs_find(rs_id);
    if (r < 0) return -1;
    if (!out) return -1;
    *out = rs_tab[r].handled;
    return 0;
}

int landlk64_rule_count(int rs_id, int *out) {
    int i, c = 0, r = landlk_rs_find(rs_id);
    if (r < 0) return -1;
    if (!out) return -1;
    for (i = 0; i < LANDLK64_MAX_RULES; i++)
        if (rule_tab[i].used && rule_tab[i].rs_id == rs_id) c++;
    *out = c;
    return 0;
}

int landlk64_is_restricted(int rs_id, int *out) {
    int r = landlk_rs_find(rs_id);
    if (r < 0) return -1;
    if (!out) return -1;
    *out = rs_tab[r].restricted;
    return 0;
}