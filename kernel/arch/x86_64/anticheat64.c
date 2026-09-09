/* 57I: anti-cheat — tarama/ihbar/ban. */
#include "arch/x86_64/longmode.h"

#define AC_MAX 256

struct ac_entry {
    int used;
    int pid;
    int banned;
    char reason[128];
};

static struct ac_entry ac_tab[AC_MAX];
static int ac_active = 0;

static void ac_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct ac_entry *ac_find(int pid) {
    int i;
    for (i = 0; i < AC_MAX; i++) {
        if (ac_tab[i].used && ac_tab[i].pid == pid) return &ac_tab[i];
    }
    return 0;
}

int anticheat64_init(void) {
    if (ac_active) return -1;
    ac_active = 1;
    return 0;
}

int anticheat64_scan(int pid) {
    if (!ac_active) return -1;
    /* Basit simülasyon: pid % 7 == 0 ise cheat */
    if (pid % 7 == 0) return 1;
    return 0;
}

int anticheat64_report(int pid, const char *reason) {
    if (!ac_active) return -1;
    struct ac_entry *e = ac_find(pid);
    int i;
    if (!e) {
        for (i = 0; i < AC_MAX; i++) {
            if (!ac_tab[i].used) {
                ac_tab[i].used = 1;
                ac_tab[i].pid = pid;
                ac_tab[i].banned = 0;
                ac_str_copy(ac_tab[i].reason, reason ? reason : "", 128);
                return 0;
            }
        }
        return -2;
    }
    ac_str_copy(e->reason, reason ? reason : "", 128);
    return 0;
}

int anticheat64_ban(int pid) {
    struct ac_entry *e = ac_find(pid);
    if (!e) return -1;
    e->banned = 1;
    return 0;
}

int anticheat64_is_banned(int pid) {
    struct ac_entry *e = ac_find(pid);
    if (!e) return -2;
    return e->banned ? 1 : 0;
}
