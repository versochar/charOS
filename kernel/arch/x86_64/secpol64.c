#include "arch/x86_64/secpol.h"
#include <string.h>

/* AppArmor/SELinux birlesik politika motoru.
 * AppArmor: yol + op izinleri (profile prefix'leri).
 * SELinux: baglam + op izinleri (rule_id'ler).
 * Kip: ENFORCE=1 (kural olmadan red), COMPLAIN=0 (kural olmadan log+izin). */

struct secpol_profile {
    int  used;
    char name[SECPOL64_NAME_MAX];
    char prefix[SECPOL64_PATH_MAX];
    int  enforce;
};

struct secpol_rule {
    int  used;
    char profile[SECPOL64_NAME_MAX];
    char path[SECPOL64_PATH_MAX];
    u32  ops;
    int  selinux; /* 1=baglam tabanli, 0=yol tabanli */
};

static struct secpol_profile profiles[SECPOL64_MAX_PROFILES];
static struct secpol_rule   rules[SECPOL64_MAX_RULES];

int secpol64_init(void) {
    memset(profiles, 0, sizeof(profiles));
    memset(rules, 0, sizeof(rules));
    return 0;
}

static int secpol_profile_find(const char *name) {
    int i;
    for (i = 0; i < SECPOL64_MAX_PROFILES; i++)
        if (profiles[i].used && strcmp(profiles[i].name, name) == 0)
            return i;
    return -1;
}

static int secpol_prefix(const char *prefix, const char *path) {
    size_t n = strlen(prefix);
    if (strncmp(prefix, path, n) != 0) return 0;
    return n > 0 && prefix[n - 1] == '/';
}

int secpol64_add_apparmor(const char *name, const char *path_prefix,
                          int enforce) {
    int i;
    if (!name || !path_prefix) return -1;
    if (secpol_profile_find(name) >= 0) return -2;
    for (i = 0; i < SECPOL64_MAX_PROFILES; i++) {
        if (!profiles[i].used) {
            strncpy(profiles[i].name, name, sizeof(profiles[i].name) - 1);
            strncpy(profiles[i].prefix, path_prefix,
                    sizeof(profiles[i].prefix) - 1);
            profiles[i].enforce = enforce ? 1 : 0;
            profiles[i].used = 1;
            return 0;
        }
    }
    return -3;
}

static int secpol_rule_add(const char *profile, const char *path, u32 ops,
                           int selinux, u32 *rule_id) {
    int i;
    if (!profile || !path) return -1;
    if (!(ops & ~(SECPOL_OP_READ | SECPOL_OP_WRITE | SECPOL_OP_EXEC))) {
        if (!ops) return -1;
    }
    for (i = 0; i < SECPOL64_MAX_RULES; i++) {
        if (!rules[i].used) {
            strncpy(rules[i].profile, profile,
                    sizeof(rules[i].profile) - 1);
            strncpy(rules[i].path, path, sizeof(rules[i].path) - 1);
            rules[i].ops = ops;
            rules[i].selinux = selinux ? 1 : 0;
            rules[i].used = 1;
            if (rule_id) *rule_id = (u32)i;
            return 0;
        }
    }
    return -2;
}

int secpol64_add_rule(const char *profile, const char *path, u32 ops,
                      int selinux, u32 *rule_id) {
    if (secpol_profile_find(profile) < 0) return -3;
    return secpol_rule_add(profile, path, ops, selinux, rule_id);
}

static int secpol_rule_hits(const char *profile, const char *path, u32 op) {
    int i;
    for (i = 0; i < SECPOL64_MAX_RULES; i++) {
        if (rules[i].used && strcmp(rules[i].profile, profile) == 0 &&
            strcmp(rules[i].path, path) == 0 && (rules[i].ops & op) == op)
            return 1;
    }
    return 0;
}

int secpol64_check(const char *profile, const char *path, u32 op) {
    int p;
    if (!profile || !path) return -1;
    p = secpol_profile_find(profile);
    if (p < 0) return -1; /* bilinmeyen profil */
    if (!(op & (SECPOL_OP_READ | SECPOL_OP_WRITE | SECPOL_OP_EXEC))) return -1;
    if (secpol_rule_hits(profile, path, op)) return 1;
    if (!secpol_prefix(profiles[p].prefix, path)) {
        /* profil yol agacinin disinda: default-allow */
        return profiles[p].enforce ? 1 : 1;
    }
    /* yol agacinin icinde: kural yoksa mode belirler */
    return profiles[p].enforce ? 0 : 1;
}

int secpol64_set_mode(const char *profile, int enforce) {
    int p = secpol_profile_find(profile);
    if (p < 0) return -1;
    profiles[p].enforce = enforce ? 1 : 0;
    return 0;
}

int secpol64_count(void) {
    int i, c = 0;
    for (i = 0; i < SECPOL64_MAX_RULES; i++)
        if (rules[i].used) c++;
    return c;
}

int secpol64_profile_count(void) {
    int i, c = 0;
    for (i = 0; i < SECPOL64_MAX_PROFILES; i++)
        if (profiles[i].used) c++;
    return c;
}