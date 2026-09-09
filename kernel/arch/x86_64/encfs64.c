/* 50I: encrypted FS — YER_TUTUCU akis sifresi (FNV anahtar-akisi).
 * UYARI: gercek gizlilik saglamaz; AES-XTS 51x kriptoda gelecek.
 * API ve nonce/uzunluk sozlesmesi simdiden sabit.
 */
#include "arch/x86_64/longmode.h"

static unsigned char encfs64_key[32];
static int encfs64_ready = 0;

int encfs64_set_key(const unsigned char key[32]) {
    int i;
    u64 sum = 0;
    if (!key) return -1;
    for (i = 0; i < 32; i++) {
        encfs64_key[i] = key[i];
        sum += key[i];
    }
    if (!sum) return -2; /* sifir anahtar yasak */
    encfs64_ready = 1;
    return 0;
}

static unsigned char encfs64_stream(u64 nonce, u64 off) {
    u64 h = 1469598103934665603ULL;
    int i;
    for (i = 0; i < 32; i++) {
        h ^= encfs64_key[i];
        h *= 1099511628211ULL;
    }
    h ^= nonce + 0x9E3779B97F4A7C15ULL;
    h ^= off * 1099511628211ULL;
    h ^= h >> 29;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 32;
    return (unsigned char)(h & 0xFF);
}

int encfs64_encrypt(u64 nonce, const void *in, void *out, u64 len) {
    const unsigned char *s;
    unsigned char *d;
    u64 i;
    if (!encfs64_ready || (!in && len) || (!out && len)) return -1;
    s = (const unsigned char *)in;
    d = (unsigned char *)out;
    for (i = 0; i < len; i++) d[i] = s[i] ^ encfs64_stream(nonce, i);
    return 0;
}

int encfs64_decrypt(u64 nonce, const void *in, void *out, u64 len) {
    return encfs64_encrypt(nonce, in, out, len); /* simetrik */
}

/* Ad karartma: FNV karmasi onalti-tabanli (16 harf). */
int encfs64_name(const char *name, char *out, int max) {
    static const char hex[] = "0123456789abcdef";
    u64 h = 1469598103934665603ULL;
    int i, n;
    if (!name || !out || max < 17) return -1;
    for (i = 0; name[i]; i++) {
        h ^= (unsigned char)name[i];
        h *= 1099511628211ULL;
    }
    n = 0;
    for (i = 15; i >= 0 && n + 1 < max; i--) {
        out[n++] = hex[(h >> (i * 4)) & 0xF];
    }
    out[n] = 0;
    return 0;
}
