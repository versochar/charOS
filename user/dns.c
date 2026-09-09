/* 14I: DNS sorgu/yanıt (RFC 1035 alt kümesi). */
#include "dns.h"

static void put16(char* p, unsigned v) { p[0] = (v >> 8) & 0xFF; p[1] = v & 0xFF; }

int dns_build_query(const char* name, unsigned short txid,
                    char* out, int maxlen) {
    if (!name || !out || maxlen < 17) return -1;
    int pos = 0;
    put16(out + pos, txid); pos += 2;
    put16(out + pos, 0x0100); pos += 2; /* RD=1 */
    put16(out + pos, 1); pos += 2;      /* QDCOUNT */
    put16(out + pos, 0); pos += 2;
    put16(out + pos, 0); pos += 2;
    put16(out + pos, 0); pos += 2;
    /* QNAME: etiketler */
    const char* s = name;
    while (*s) {
        const char* dot = s;
        int ll = 0;
        while (*dot && *dot != '.') { dot++; ll++; }
        if (ll == 0 || ll > 63) return -1;
        if (pos + 1 + ll + 5 > maxlen) return -1;
        out[pos++] = (char)ll;
        for (int i = 0; i < ll; i++) out[pos++] = s[i];
        s = (*dot == '.') ? dot + 1 : dot;
    }
    if (pos + 5 > maxlen) return -1;
    out[pos++] = 0;
    put16(out + pos, 1); pos += 2; /* QTYPE=A */
    put16(out + pos, 1); pos += 2; /* QCLASS=IN */
    return pos;
}

/* İsim atlama (etiketler + 0xC0 işaretçileri). Dönüş: sonraki ofset / -1. */
static int dns_skip_name(const char* m, int len, int off) {
    int jumps = 0;
    while (off < len) {
        unsigned char b = (unsigned char)m[off];
        if (b == 0) return off + 1;
        if ((b & 0xC0) == 0xC0) {
            if (off + 1 >= len) return -1;
            if (++jumps > 8) return -1;
            int ptr = ((b & 0x3F) << 8) | (unsigned char)m[off + 1];
            if (ptr >= len) return -1;
            /* İşaretçi hedefi de geçerli isim olmalı (derinlik sınırlı) */
            int t = ptr, dj = 0;
            while (t < len) {
                unsigned char tb = (unsigned char)m[t];
                if (tb == 0) break;
                if ((tb & 0xC0) == 0xC0) {
                    if (++dj > 8) return -1;
                    t = (((tb & 0x3F) << 8) | (unsigned char)m[t + 1]);
                    continue;
                }
                t += 1 + tb;
            }
            return off + 2;
        }
        off += 1 + b;
    }
    return -1;
}

int dns_parse_response(const char* msg, int len, unsigned char ip[4]) {
    if (!msg || len < 12 || !ip) return -1;
    unsigned qd = ((unsigned char)msg[4] << 8) | (unsigned char)msg[5];
    unsigned an = ((unsigned char)msg[6] << 8) | (unsigned char)msg[7];
    if (!(msg[2] & 0x80)) return -1; /* QR=1 olmalı */
    if (qd != 1 || an < 1) return -1;
    int off = 12;
    off = dns_skip_name(msg, len, off);
    if (off < 0 || off + 4 > len) return -1;
    off += 4; /* QTYPE+QCLASS */
    /* İlk yanıt */
    off = dns_skip_name(msg, len, off);
    if (off < 0 || off + 10 > len) return -1;
    unsigned type = ((unsigned char)msg[off] << 8) | (unsigned char)msg[off + 1];
    /* unsigned class = ...; (IN beklenir ama esnek) */
    unsigned rdlen = ((unsigned char)msg[off + 8] << 8) | (unsigned char)msg[off + 9];
    off += 10;
    if (type != 1 || rdlen != 4) return -1;
    if (off + 4 > len) return -1;
    for (int i = 0; i < 4; i++) ip[i] = (unsigned char)msg[off + i];
    return 0;
}
