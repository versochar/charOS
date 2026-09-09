/* 43F: DNS cozucu — sorgu kurma + sikistirmali yanit ayrıştırma (A). */
#include "arch/x86_64/longmode.h"

#define DNS64_MAX_CACHE 16

struct dns64_entry {
    int used;
    char name[64];
    u32 ip;
};

static struct dns64_entry dns64_cache[DNS64_MAX_CACHE];

static void dns_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int dns_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        if (!ca) return 1;
    }
}

/* Baslik(12B) + etiketli ad + QTYPE/QCLASS; donus boy. */
int dns64_query(const char *name, unsigned char *out, int max) {
    int pos = 12, i;
    if (!name || !out || max < 18) return -1;
    for (i = 0; i < 12; i++) out[i] = 0;
    out[0] = 0x12;
    out[1] = 0x34; /* id */
    out[2] = 0x01; /* recursion desired */
    while (*name) {
        const char *dot = name;
        int len = 0;
        while (*dot && *dot != '.') {
            dot++;
            len++;
        }
        if (!len || len > 63 || pos + 1 + len + 4 >= max) return -2;
        out[pos++] = (unsigned char)len;
        for (i = 0; i < len; i++) out[pos++] = (unsigned char)name[i];
        name += len;
        if (*name == '.') name++;
    }
    if (pos + 5 >= max) return -2;
    out[pos++] = 0;
    out[pos++] = 0;
    out[pos++] = 1; /* A */
    out[pos++] = 0;
    out[pos++] = 1; /* IN */
    return pos;
}

/* Ad cozumleyici (isaretci 0xC0xx destekli); donus SAHADA tuketilen
 * bayt (isaretcinin gosterdigi hedefin boyu DEGIL). */
static int dns64_skip_name(const unsigned char *p, int len, int pos) {
    int start = pos, end = pos, guard = 0, jumped = 0;
    while (pos < len && guard++ < 64) {
        unsigned char c = p[pos];
        if ((c & 0xC0) == 0xC0) {
            if (pos + 1 >= len) return -1;
            if (!jumped) end = pos + 2;
            pos = ((c & 0x3F) << 8) | p[pos + 1];
            jumped = 1;
            continue;
        }
        if (!c) {
            if (!jumped) end = pos + 1;
            return end - start;
        }
        if (pos + 1 + c > len) return -1;
        pos += 1 + c;
    }
    return -1;
}

int dns64_parse(const unsigned char *resp, int len, u32 *ips, int max) {
    int pos, qd, an, i, n = 0;
    if (!resp || len < 12 || !ips || max <= 0) return -1;
    if ((resp[2] & 0x80) == 0) return -2; /* yanit degil */
    if ((resp[3] & 0x0F) != 0) return -3; /* rcode */
    qd = (resp[4] << 8) | resp[5];
    an = (resp[6] << 8) | resp[7];
    pos = 12;
    for (i = 0; i < qd; i++) {
        int s = dns64_skip_name(resp, len, pos);
        if (s < 0 || pos + s + 4 > len) return -4;
        pos += s + 4;
    }
    for (i = 0; i < an && n < max; i++) {
        int s = dns64_skip_name(resp, len, pos);
        u32 type, rdlen;
        if (s < 0 || pos + s + 10 > len) break;
        pos += s;
        type = ((u32)resp[pos] << 8) | resp[pos + 1];
        rdlen = ((u32)resp[pos + 8] << 8) | resp[pos + 9];
        pos += 10;
        if (pos + (int)rdlen > len) break;
        if (type == 1 && rdlen == 4) {
            ips[n++] = ((u32)resp[pos] << 24) | ((u32)resp[pos + 1] << 16) |
                       ((u32)resp[pos + 2] << 8) | resp[pos + 3];
        }
        pos += rdlen;
    }
    return n;
}

int dns64_cached(const char *name, u32 *ip_out) {
    int i;
    if (!name) return -1;
    for (i = 0; i < DNS64_MAX_CACHE; i++) {
        if (dns64_cache[i].used && dns_str_eq(dns64_cache[i].name, name)) {
            if (ip_out) *ip_out = dns64_cache[i].ip;
            return 0;
        }
    }
    return -2;
}

/* Test/istek icin onbellege yaz (cozumleyici dahili kullanir). */
int dns64_cache_add(const char *name, u32 ip) {
    int i;
    if (!name) return -1;
    for (i = 0; i < DNS64_MAX_CACHE; i++) {
        if (!dns64_cache[i].used) {
            dns64_cache[i].used = 1;
            dns_str_copy(dns64_cache[i].name, name, 64);
            dns64_cache[i].ip = ip;
            return 0;
        }
    }
    return -2;
}
