/* 37I: module signing — FNV-1a karma + anahtarlik + taint bayragi. */
#include "arch/x86_64/longmode.h"

#define MODSIGN64_MAX_KEYS 16

struct modsign64_key {
    int used;
    int id;
    u64 hash;
};

static struct modsign64_key modsign64_keys[MODSIGN64_MAX_KEYS];
static int modsign64_taint = 0;

u64 modsign64_hash(const void *data, u64 len) {
    const unsigned char *p = (const unsigned char *)data;
    u64 h = 1469598103934665603ULL;
    u64 i;
    if (!p) return 0;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h ? h : 1;
}

int modsign64_add_key(int id, u64 hash) {
    int i;
    if (!hash) return -1;
    for (i = 0; i < MODSIGN64_MAX_KEYS; i++) {
        if (modsign64_keys[i].used && modsign64_keys[i].id == id) {
            modsign64_keys[i].hash = hash;
            return 0;
        }
    }
    for (i = 0; i < MODSIGN64_MAX_KEYS; i++) {
        if (!modsign64_keys[i].used) {
            modsign64_keys[i].used = 1;
            modsign64_keys[i].id = id;
            modsign64_keys[i].hash = hash;
            return 0;
        }
    }
    return -2;
}

/* 0=dogrulandi, <0 red (taint bayragi kurulur) */
int modsign64_verify(const void *data, u64 len, int key_id) {
    u64 h;
    int i;
    if (!data) return -1;
    h = modsign64_hash(data, len);
    for (i = 0; i < MODSIGN64_MAX_KEYS; i++) {
        if (modsign64_keys[i].used && modsign64_keys[i].id == key_id) {
            if (modsign64_keys[i].hash == h) return 0;
            modsign64_taint = 1;
            return -2;
        }
    }
    modsign64_taint = 1;
    return -3; /* anahtar yok */
}

int modsign64_tainted(void) { return modsign64_taint; }
