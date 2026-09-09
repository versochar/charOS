/* 43H: SLAAC — RA one Ek ayrıştırma + EUI-64 + DAD durumu. */
#include "arch/x86_64/longmode.h"

static unsigned char slaac64_ip[16];
static int slaac64_has_ip = 0;
static int slaac64_dad_state = 0; /* 0=bos,1=bekliyor,2=ok,3=catisma */

int slaac64_eui64(const unsigned char mac[6],
                  const unsigned char prefix[8], unsigned char out[16]) {
    int i;
    if (!mac || !prefix || !out) return -1;
    for (i = 0; i < 8; i++) out[i] = prefix[i];
    out[8] = mac[0] ^ 0x02; /* U/L cevir */
    out[9] = mac[1];
    out[10] = mac[2];
    out[11] = 0xFF;
    out[12] = 0xFE;
    out[13] = mac[3];
    out[14] = mac[4];
    out[15] = mac[5];
    return 0;
}

/* RA: tur=134, secenek 3 = one Ek (uzunluk 4 => 32B: bayt+one Ek+omur+one Ek). */
int slaac64_ra_parse(const unsigned char *ra, int len,
                     unsigned char prefix[8], u32 *lifetime) {
    int pos;
    if (!ra || len < 16) return -1;
    if (ra[0] != 134) return -2;
    pos = 16;
    while (pos + 2 <= len) {
        unsigned char type = ra[pos];
        unsigned char units = ra[pos + 1];
        int optlen;
        if (!units) break;
        optlen = units * 8;
        if (pos + optlen > len) break;
        if (type == 3 && optlen >= 32) {
            int i;
            u32 life = ((u32)ra[pos + 4] << 24) | ((u32)ra[pos + 5] << 16) |
                       ((u32)ra[pos + 6] << 8) | ra[pos + 7];
            if (life == 0) {
                pos += optlen;
                continue;
            }
            if (prefix)
                for (i = 0; i < 8; i++) prefix[i] = ra[pos + 16 + i];
            if (lifetime) *lifetime = life;
            return 0;
        }
        pos += optlen;
    }
    return -3; /* one Ek yok */
}

int slaac64_dad_start(const unsigned char ip[16]) {
    int i;
    if (!ip) return -1;
    for (i = 0; i < 16; i++) slaac64_ip[i] = ip[i];
    slaac64_has_ip = 0;
    slaac64_dad_state = 1;
    return 0;
}

int slaac64_dad_ok(void) {
    if (slaac64_dad_state != 1) return -1;
    slaac64_dad_state = 2;
    slaac64_has_ip = 1;
    return 0;
}

int slaac64_dad_conflict(void) {
    if (slaac64_dad_state != 1) return -1;
    slaac64_dad_state = 3;
    slaac64_has_ip = 0;
    return 0;
}
