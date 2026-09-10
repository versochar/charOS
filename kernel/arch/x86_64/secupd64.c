#include "arch/x86_64/secupd.h"
#include <string.h>

struct secupd_key {
    int  used;
    int  id;
    u64  secret;
};
struct secupd_entry {
    char name[SECUPD64_NAME_MAX];
    char ver[SECUPD64_NAME_MAX];
    u64  payload_hash;
    u64  sig;
    int  key_id;
    u8   verify_ok;
};
static struct secupd_key secupd_keys[SECUPD64_MAX_KEYS];
static struct secupd_entry secupd_tab[SECUPD64_MAX_KEYS];
static int secupd_n = 0;
static int secupd_policy_mode = SECUPD_POLICY_ENFORCED;

static u64 secupd_hash(const void *data, u64 len, u64 secret) {
    const unsigned char *p = (const unsigned char *)data;
    u64 h = 1469598103934665603ULL ^ secret;
    u64 i;
    if (!p) return 0;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    h ^= secret >> 1;
    return h ? h : 1;
}

static u64 secupd_mac(const char *name, const char *ver, u64 payload_hash,
                      u64 secret) {
    /* ad + surum + icerik ozeti birlestir */
    unsigned char mix[SECUPD64_NAME_MAX * 2 + 8];
    size_t ln = strlen(name);
    size_t lv = strlen(ver);
    size_t i;
    u64 h;
    if (ln > SECUPD64_NAME_MAX - 1) ln = SECUPD64_NAME_MAX - 1;
    if (lv > SECUPD64_NAME_MAX - 1) lv = SECUPD64_NAME_MAX - 1;
    for (i = 0; i < ln; i++) mix[i] = (unsigned char)name[i];
    for (i = 0; i < lv; i++) mix[ln + i] = (unsigned char)ver[i];
    for (i = 0; i < 8; i++)
        mix[ln + lv + i] = (unsigned char)(payload_hash >> (i * 8));
    h = secupd_hash(mix, ln + lv + 8, secret);
    return h ? h : 1;
}

static int secupd_find_key(int id, u64 *secret_out) {
    int i;
    for (i = 0; i < SECUPD64_MAX_KEYS; i++) {
        if (secupd_keys[i].used && secupd_keys[i].id == id) {
            if (secret_out) *secret_out = secupd_keys[i].secret;
            return 0;
        }
    }
    return -1;
}

int secupd64_init(void) {
    int i;
    secupd_n = 0;
    secupd_policy_mode = SECUPD_POLICY_ENFORCED;
    for (i = 0; i < SECUPD64_MAX_KEYS; i++) {
        secupd_keys[i].used = 0;
        secupd_keys[i].id = 0;
        secupd_keys[i].secret = 0;
        memset(&secupd_tab[i], 0, sizeof(secupd_tab[i]));
    }
    return 0;
}

int secupd64_set_policy(int policy) {
    if (policy < SECUPD_POLICY_RELAXED || policy > SECUPD_POLICY_STRICT)
        return -1;
    secupd_policy_mode = policy;
    return 0;
}

int secupd64_add_key(int id, u64 secret) {
    int i;
    if (!secret) return -1;
    if (secupd_find_key(id, 0) == 0) return -2; /* zaten var */
    for (i = 0; i < SECUPD64_MAX_KEYS; i++) {
        if (!secupd_keys[i].used) {
            secupd_keys[i].used = 1;
            secupd_keys[i].id = id;
            secupd_keys[i].secret = secret;
            return 0;
        }
    }
    return -3;
}

u64 secupd64_sign(const char *name, const char *ver, u64 payload_hash,
                  int key_id) {
    u64 secret;
    if (!name || !ver || !payload_hash) return 0;
    if (secupd_find_key(key_id, &secret) != 0) return 0;
    return secupd_mac(name, ver, payload_hash, secret);
}

int secupd64_stage(const char *name, const char *ver, u64 payload_hash,
                   u64 sig, int key_id) {
    u64 expected;
    if (!name || !ver || !payload_hash || !sig) return -1;
    if (secupd_n >= SECUPD64_MAX_KEYS) return -2;
    /* imzanin gercekten bir kayitli anahtarla uretilip uretilmedigini dogrula */
    if (secupd_find_key(key_id, &expected) != 0) return -3;
    if (secupd_mac(name, ver, payload_hash, expected) != sig) return -4;
    memset(&secupd_tab[secupd_n], 0, sizeof(secupd_tab[secupd_n]));
    strncpy(secupd_tab[secupd_n].name, name,
            sizeof(secupd_tab[secupd_n].name) - 1);
    strncpy(secupd_tab[secupd_n].ver, ver,
            sizeof(secupd_tab[secupd_n].ver) - 1);
    secupd_tab[secupd_n].payload_hash = payload_hash;
    secupd_tab[secupd_n].sig = sig;
    secupd_tab[secupd_n].key_id = key_id;
    secupd_tab[secupd_n].verify_ok = 1;
    secupd_n++;
    return 0;
}

int secupd64_verify(const char *name, const char *ver) {
    int i;
    u64 expected;
    if (!name || !ver) return -1;
    for (i = 0; i < secupd_n; i++) {
        if (strcmp(secupd_tab[i].name, name) == 0 &&
            strcmp(secupd_tab[i].ver, ver) == 0) {
            if (secupd_find_key(secupd_tab[i].key_id, &expected) != 0)
                return -2;
            if (secupd_mac(name, ver, secupd_tab[i].payload_hash,
                           expected) != secupd_tab[i].sig)
                return -3;
            return 0;
        }
    }
    return -4; /* kayit yok */
}

int secupd64_apply_ok(const char *name, const char *ver) {
    int i;
    if (!name || !ver) return -1;
    if (secupd_policy_mode == SECUPD_POLICY_RELAXED) {
        /* imza bilinmiyorsa da uygulanabilir */
        return 0;
    }
    for (i = 0; i < secupd_n; i++) {
        if (strcmp(secupd_tab[i].name, name) == 0 &&
            strcmp(secupd_tab[i].ver, ver) == 0 && secupd_tab[i].verify_ok)
            return 0;
    }
    return -2; /* imzali dogrulanmis kayit yok */
}

int secupd64_policy(int *out) {
    if (!out) return -1;
    *out = secupd_policy_mode;
    return 0;
}

int secupd64_staged_count(void) {
    return secupd_n;
}