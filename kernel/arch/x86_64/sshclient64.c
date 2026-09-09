/* 54E: ssh client — ikili paket + surum + kexinit iskeleti. */
#include "arch/x86_64/longmode.h"

int sshclient64_banner(const char *line, char *ver_out, int max) {
    int i = 0, j;
    const char *p;
    if (!line || !ver_out || max <= 0) return -1;
    /* "SSH-2.0-..." bicimi sart */
    if (line[0] != 'S' || line[1] != 'S' || line[2] != 'H' ||
        line[3] != '-')
        return -2;
    p = line + 4;
    while (*p && *p != '-' && *p != '\n' && i + 1 < max)
        ver_out[i++] = *p++;
    ver_out[i] = 0;
    if (i < 3) return -3;
    for (j = 0; ver_out[j]; j++) {
    }
    (void)j;
    return 0;
}

/* Ikili paket: boy(4) + dolgu-boy(1) + yuk + dolgu + mac(16B sifir).
 * Donus toplam boy. */
int sshclient64_packet(u8 type, const void *payload, u64 len,
                       unsigned char *out, int max) {
    const unsigned char *p;
    u64 total, padlen, i, o = 0;
    if (!out || max <= 0) return -1;
    if (!payload) len = 0;
    total = 1 + len;
    padlen = 8 - ((total + 5) % 8);
    if (padlen < 4) padlen += 8;
    total += padlen + 16; /* mac */
    if (total + 4 > (u64)max) return -2;
    out[o++] = (unsigned char)((total >> 24) & 0xFF);
    out[o++] = (unsigned char)((total >> 16) & 0xFF);
    out[o++] = (unsigned char)((total >> 8) & 0xFF);
    out[o++] = (unsigned char)(total & 0xFF);
    out[o++] = (unsigned char)padlen;
    out[o++] = type;
    p = (const unsigned char *)payload;
    for (i = 0; i < len; i++) out[o++] = p[i];
    for (i = 0; i < padlen; i++) out[o++] = 0;
    for (i = 0; i < 16; i++) out[o++] = 0; /* mac stub */
    return (int)o;
}

/* KEXINIT: tip(20) + 16B cerez + liste bosluklari (skeleton sabit). */
int sshclient64_kexinit(unsigned char *out, int max) {
    int i, o = 0;
    unsigned char zero[4] = {0, 0, 0, 0};
    if (!out || max < 64) return -1;
    /* Basit kexinit yukü: tip + cerez + bos listeler */
    if (sshclient64_packet(20, zero, 0, out, max) < 0) return -2;
    for (i = 0; i < 16; i++) out[6 + i] = (unsigned char)(0xA0 + i);
    (void)o;
    return 0;
}
