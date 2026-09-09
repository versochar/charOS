/* 38C: sudo — kural tablosu + damga (timestamp) + komut esleme.
 * Komut "*" jokerdir; hedef kullanici eslesmelidir.
 */
#include "arch/x86_64/longmode.h"

#define SUDO64_MAX_RULES 32
#define SUDO64_MAX_STAMPS 32

struct sudo64_rule {
    int used;
    char user[32];
    char target[32];
    char command[64];
    int nopasswd;
};

struct sudo64_stamp {
    int used;
    char user[32];
    u64 time;
};

static struct sudo64_rule sudo64_rules[SUDO64_MAX_RULES];
static struct sudo64_stamp sudo64_stamps[SUDO64_MAX_STAMPS];

static void sudo_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int sudo_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* command "*" ise one Ekli hedef icin joker */
static int sudo_cmd_match(const char *rule, const char *cmd) {
    if (sudo_str_eq(rule, "*")) return 1;
    return sudo_str_eq(rule, cmd);
}

int sudo64_rule_add(const char *user, const char *target,
                    const char *command, int nopasswd) {
    int i;
    if (!user || !target || !command) return -1;
    for (i = 0; i < SUDO64_MAX_RULES; i++) {
        if (!sudo64_rules[i].used) {
            sudo64_rules[i].used = 1;
            sudo_str_copy(sudo64_rules[i].user, user, 32);
            sudo_str_copy(sudo64_rules[i].target, target, 32);
            sudo_str_copy(sudo64_rules[i].command, command, 64);
            sudo64_rules[i].nopasswd = nopasswd ? 1 : 0;
            return 0;
        }
    }
    return -2;
}

/* 1=serbest, 0=engelli. need_pass: 1=sifre gerekli. */
int sudo64_check(const char *user, const char *target, const char *command,
                 int *need_pass) {
    int i;
    if (!user || !target || !command) return 0;
    for (i = 0; i < SUDO64_MAX_RULES; i++) {
        struct sudo64_rule *r = &sudo64_rules[i];
        if (!r->used) continue;
        if (!sudo_str_eq(r->user, user)) continue;
        if (!sudo_str_eq(r->target, target)) continue;
        if (!sudo_cmd_match(r->command, command)) continue;
        if (need_pass) *need_pass = r->nopasswd ? 0 : 1;
        return 1;
    }
    return 0;
}

int sudo64_stamp(const char *user, u64 now) {
    int i;
    if (!user) return -1;
    for (i = 0; i < SUDO64_MAX_STAMPS; i++) {
        if (sudo64_stamps[i].used) {
            int j, same = 1;
            for (j = 0; user[j] || sudo64_stamps[i].user[j]; j++) {
                if (user[j] != sudo64_stamps[i].user[j]) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                sudo64_stamps[i].time = now;
                return 0;
            }
        }
    }
    for (i = 0; i < SUDO64_MAX_STAMPS; i++) {
        if (!sudo64_stamps[i].used) {
            sudo64_stamps[i].used = 1;
            sudo_str_copy(sudo64_stamps[i].user, user, 32);
            sudo64_stamps[i].time = now;
            return 0;
        }
    }
    return -2;
}

int sudo64_stamp_valid(const char *user, u64 now, u64 timeout) {
    int i, j;
    if (!user) return 0;
    for (i = 0; i < SUDO64_MAX_STAMPS; i++) {
        int same;
        if (!sudo64_stamps[i].used) continue;
        same = 1;
        for (j = 0; user[j] || sudo64_stamps[i].user[j]; j++) {
            if (user[j] != sudo64_stamps[i].user[j]) {
                same = 0;
                break;
            }
        }
        if (!same) continue;
        if (now < sudo64_stamps[i].time) return 0; /* saat geriledi */
        return (now - sudo64_stamps[i].time) <= timeout ? 1 : 0;
    }
    return 0;
}
