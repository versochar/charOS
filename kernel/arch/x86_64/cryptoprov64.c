/* 53E: crypto provider — karma/sifre kaydi + yerlesikler.
 * Yerlesik "fnv1a64" (karma) ve "xorstream" (YER_TUTUCU sifre).
 */
#include "arch/x86_64/longmode.h"

#define CRYPTOPROV64_MAX_HASH 8
#define CRYPTOPROV64_MAX_CIPHER 8

struct cryptoprov64_hash {
    int used;
    char name[32];
    cryptoprov64_hashfn fn;
};

struct cryptoprov64_cipher {
    int used;
    char name[32];
    cryptoprov64_cryptfn fn;
};

static struct cryptoprov64_hash cryptoprov64_hashes[CRYPTOPROV64_MAX_HASH];
static struct cryptoprov64_cipher
    cryptoprov64_ciphers[CRYPTOPROV64_MAX_CIPHER];

static void crypto_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int crypto_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static int crypto_fnv1a(const void *data, u64 len, u64 *out) {
    const unsigned char *p = (const unsigned char *)data;
    u64 h = 1469598103934665603ULL, i;
    if (!p) return -1;
    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    if (out) *out = h;
    return 0;
}

static int crypto_xorstream(const unsigned char *key,
                            const unsigned char *in, unsigned char *out,
                            u64 len) {
    u64 i;
    if (!key || !in || !out) return -1;
    for (i = 0; i < len; i++) out[i] = in[i] ^ key[i % 32];
    return 0;
}

static void cryptoprov64_builtin(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    cryptoprov64_register_hash("fnv1a64", crypto_fnv1a);
    cryptoprov64_register_cipher("xorstream", crypto_xorstream);
}

int cryptoprov64_register_hash(const char *name,
                               cryptoprov64_hashfn fn) {
    int i;
    if (!name || !fn) return -1;
    for (i = 0; i < CRYPTOPROV64_MAX_HASH; i++) {
        if (!cryptoprov64_hashes[i].used) {
            cryptoprov64_hashes[i].used = 1;
            crypto_str_copy(cryptoprov64_hashes[i].name, name, 32);
            cryptoprov64_hashes[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

int cryptoprov64_register_cipher(const char *name,
                                 cryptoprov64_cryptfn fn) {
    int i;
    if (!name || !fn) return -1;
    for (i = 0; i < CRYPTOPROV64_MAX_CIPHER; i++) {
        if (!cryptoprov64_ciphers[i].used) {
            cryptoprov64_ciphers[i].used = 1;
            crypto_str_copy(cryptoprov64_ciphers[i].name, name, 32);
            cryptoprov64_ciphers[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

int cryptoprov64_hash(const char *name, const void *data, u64 len,
                      u64 *out) {
    int i;
    cryptoprov64_builtin();
    if (!name) return -1;
    for (i = 0; i < CRYPTOPROV64_MAX_HASH; i++) {
        if (cryptoprov64_hashes[i].used &&
            crypto_str_eq(cryptoprov64_hashes[i].name, name))
            return cryptoprov64_hashes[i].fn(data, len, out);
    }
    return -2;
}

int cryptoprov64_crypt(const char *name, const unsigned char *key,
                       const unsigned char *in, unsigned char *out,
                       u64 len) {
    int i;
    cryptoprov64_builtin();
    if (!name) return -1;
    for (i = 0; i < CRYPTOPROV64_MAX_CIPHER; i++) {
        if (cryptoprov64_ciphers[i].used &&
            crypto_str_eq(cryptoprov64_ciphers[i].name, name))
            return cryptoprov64_ciphers[i].fn(key, in, out, len);
    }
    return -2;
}
