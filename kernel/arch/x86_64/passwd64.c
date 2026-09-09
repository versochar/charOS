/* 38A: passwd/shadow — kullanici kaydi + tuzlu karma + sure dogrulama. */
#include "arch/x86_64/longmode.h"

#define PASSWD64_MAX 32

struct passwd64_entry {
    int used;
    char user[32];
    u32 uid;
    u32 gid;
    char home[64];
    char shell[64];
};

struct shadow64_entry {
    int used;
    char user[32];
    u64 hash;
    u64 lastchg;
    u64 maxdays; /* 0 = suresiz */
};

static struct passwd64_entry passwd64_tab[PASSWD64_MAX];
static struct shadow64_entry shadow64_tab[PASSWD64_MAX];

static void pstr_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int pstr_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* Tuzlu karma (skeleton: FNV-1a; gercek kripto 39x'te). Deterministik. */
u64 passwd64_hash(const char *pw, const char *salt) {
    u64 h = 1469598103934665603ULL;
    u64 i;
    if (!pw) return 0;
    for (i = 0; pw[i]; i++) {
        h ^= (unsigned char)pw[i];
        h *= 1099511628211ULL;
    }
    h ^= 0xFF;
    if (salt) {
        for (i = 0; salt[i]; i++) {
            h ^= (unsigned char)salt[i];
            h *= 1099511628211ULL;
        }
    }
    return h ? h : 1;
}

int passwd64_add(const char *user, u32 uid, u32 gid, const char *home,
                 const char *shell) {
    int i;
    if (!user || !user[0]) return -1;
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (passwd64_tab[i].used && pstr_eq(passwd64_tab[i].user, user)) {
            passwd64_tab[i].uid = uid;
            passwd64_tab[i].gid = gid;
            if (home) pstr_copy(passwd64_tab[i].home, home, 64);
            if (shell) pstr_copy(passwd64_tab[i].shell, shell, 64);
            return 0;
        }
    }
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (!passwd64_tab[i].used) {
            passwd64_tab[i].used = 1;
            pstr_copy(passwd64_tab[i].user, user, 32);
            passwd64_tab[i].uid = uid;
            passwd64_tab[i].gid = gid;
            pstr_copy(passwd64_tab[i].home, home ? home : "/", 64);
            pstr_copy(passwd64_tab[i].shell, shell ? shell : "/bin/sh", 64);
            return 0;
        }
    }
    return -2;
}

int passwd64_find(const char *user, u32 *uid, u32 *gid) {
    int i;
    if (!user) return -1;
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (passwd64_tab[i].used && pstr_eq(passwd64_tab[i].user, user)) {
            if (uid) *uid = passwd64_tab[i].uid;
            if (gid) *gid = passwd64_tab[i].gid;
            return 0;
        }
    }
    return -2; /* yok */
}

int shadow64_set(const char *user, u64 hash, u64 lastchg, u64 maxdays) {
    int i;
    if (!user || !hash) return -1;
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (shadow64_tab[i].used && pstr_eq(shadow64_tab[i].user, user)) {
            shadow64_tab[i].hash = hash;
            shadow64_tab[i].lastchg = lastchg;
            shadow64_tab[i].maxdays = maxdays;
            return 0;
        }
    }
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (!shadow64_tab[i].used) {
            shadow64_tab[i].used = 1;
            pstr_copy(shadow64_tab[i].user, user, 32);
            shadow64_tab[i].hash = hash;
            shadow64_tab[i].lastchg = lastchg;
            shadow64_tab[i].maxdays = maxdays;
            return 0;
        }
    }
    return -2;
}

/* 0=giris serbest, <0 red (eslesme/sure). */
int shadow64_check(const char *user, u64 hash, u64 today) {
    int i;
    if (!user) return -1;
    for (i = 0; i < PASSWD64_MAX; i++) {
        if (shadow64_tab[i].used && pstr_eq(shadow64_tab[i].user, user)) {
            if (shadow64_tab[i].hash != hash) return -2;
            if (shadow64_tab[i].maxdays &&
                today > shadow64_tab[i].lastchg + shadow64_tab[i].maxdays)
                return -3; /* suresi dolmus */
            return 0;
        }
    }
    return -4; /* kayit yok */
}
