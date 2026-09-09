/* 41D: imza — gizli-anahtarli karma imzasi + dogrulama. */
#include "arch/x86_64/longmode.h"

#define SIGN64_MAX_KEYS 16

static struct {
    int used;
    int id;
    u64 secret;
} sign64_keys[SIGN64_MAX_KEYS];

static u64 sign64_hash(const void *data, u64 len, u64 secret) {
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

int sign64_add_key(int id, u64 secret) {
    int i;
    if (!secret) return -1;
    for (i = 0; i < SIGN64_MAX_KEYS; i++) {
        if (sign64_keys[i].used && sign64_keys[i].id == id) {
            sign64_keys[i].secret = secret;
            return 0;
        }
    }
    for (i = 0; i < SIGN64_MAX_KEYS; i++) {
        if (!sign64_keys[i].used) {
            sign64_keys[i].used = 1;
            sign64_keys[i].id = id;
            sign64_keys[i].secret = secret;
            return 0;
        }
    }
    return -2;
}

u64 sign64_sign(const void *data, u64 len, int key_id) {
    int i;
    if (!data) return 0;
    for (i = 0; i < SIGN64_MAX_KEYS; i++) {
        if (sign64_keys[i].used && sign64_keys[i].id == key_id)
            return sign64_hash(data, len, sign64_keys[i].secret);
    }
    return 0;
}

int sign64_verify(const void *data, u64 len, u64 sig, int key_id) {
    int i;
    if (!data || !sig) return -1;
    for (i = 0; i < SIGN64_MAX_KEYS; i++) {
        if (sign64_keys[i].used && sign64_keys[i].id == key_id)
            return sign64_hash(data, len, sign64_keys[i].secret) == sig
                       ? 0
                       : -2;
    }
    return -3; /* anahtar yok */
}
