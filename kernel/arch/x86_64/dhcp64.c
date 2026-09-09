/* 43E: DHCP istemci — durum makinesi + mesaj kurma/ayristirma.
 * Secenekler: 1=mask, 3=router, 6=dns, 51=lease, 53=tur, 255=son.
 */
#include "arch/x86_64/longmode.h"

#define DHCP64_MAGIC 0x63825363UL
#define DHCP64_DISCOVER 1
#define DHCP64_OFFER 2
#define DHCP64_REQUEST 3
#define DHCP64_ACK 5

static u32 dhcp64_ip = 0;
static u32 dhcp64_lease = 0;
static int dhcp64_state = 0; /* 0=bos,1=kesif,2=istek,3=bagli */

static void dhcp_wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

/* BOOTP govdesi (236B) + magic + tur secenegi; donus toplam boy. */
static int dhcp64_build(unsigned char *out, int max, u32 xid, int type,
                        u32 req_ip) {
    int pos;
    if (!out || max < 244) return -1;
    for (pos = 0; pos < 236; pos++) out[pos] = 0;
    out[0] = 1;    /* op: BOOTREQUEST */
    out[1] = 1;    /* htype: ethernet */
    out[2] = 6;    /* hlen */
    dhcp_wr32(out + 4, xid);
    dhcp_wr32(out + 12, req_ip); /* ciaddr (REQUEST'te) */
    out[236] = (unsigned char)((DHCP64_MAGIC >> 24) & 0xFF);
    out[237] = (unsigned char)((DHCP64_MAGIC >> 16) & 0xFF);
    out[238] = (unsigned char)((DHCP64_MAGIC >> 8) & 0xFF);
    out[239] = (unsigned char)(DHCP64_MAGIC & 0xFF);
    out[240] = 53;
    out[241] = 1;
    out[242] = (unsigned char)type;
    if (type == DHCP64_REQUEST && req_ip) {
        /* Secenek 50: istenen IP */
        out[243] = 50;
        out[244] = 4;
        out[245] = (unsigned char)((req_ip >> 24) & 0xFF);
        out[246] = (unsigned char)((req_ip >> 16) & 0xFF);
        out[247] = (unsigned char)((req_ip >> 8) & 0xFF);
        out[248] = (unsigned char)(req_ip & 0xFF);
        out[249] = 255;
        return 250;
    }
    out[243] = 255;
    return 244;
}

int dhcp64_discover(u32 xid, unsigned char *out, int max) {
    int rc = dhcp64_build(out, max, xid, DHCP64_DISCOVER, 0);
    if (rc < 0) return rc;
    dhcp64_state = 1;
    return rc;
}

int dhcp64_request(u32 xid, u32 ip, unsigned char *out, int max) {
    int rc = dhcp64_build(out, max, xid, DHCP64_REQUEST, ip);
    if (rc < 0) return rc;
    dhcp64_state = 2;
    return rc;
}

static u32 dhcp_rd32(const unsigned char *p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) |
           (u32)p[3];
}

int dhcp64_offer_parse(const unsigned char *msg, int len, u32 *ip,
                       u32 *mask, u32 *gw, u32 *dns, u32 *lease) {
    u32 yiaddr;
    int pos, type = 0;
    if (!msg || len < 244) return -1;
    if (msg[236] != 0x63 || msg[237] != 0x82 || msg[238] != 0x53 ||
        msg[239] != 0x63)
        return -2;
    yiaddr = dhcp_rd32(msg + 16);
    pos = 240;
    while (pos < len) {
        u8 opt = msg[pos];
        u8 olen;
        if (opt == 255) break;
        if (opt == 0) {
            pos++;
            continue;
        }
        if (pos + 1 >= len) break;
        olen = msg[pos + 1];
        if (pos + 2 + olen > len) break;
        if (opt == 53 && olen >= 1)
            type = msg[pos + 2];
        else if (opt == 1 && olen >= 4 && mask)
            *mask = dhcp_rd32(msg + pos + 2);
        else if (opt == 3 && olen >= 4 && gw)
            *gw = dhcp_rd32(msg + pos + 2);
        else if (opt == 6 && olen >= 4 && dns)
            *dns = dhcp_rd32(msg + pos + 2);
        else if (opt == 51 && olen >= 4 && lease)
            *lease = dhcp_rd32(msg + pos + 2);
        pos += 2 + olen;
    }
    if (type != DHCP64_OFFER && type != DHCP64_ACK) return -3;
    if (ip) *ip = yiaddr;
    return 0;
}

int dhcp64_bound(u32 ip, u32 lease) {
    if (!ip) return -1;
    dhcp64_ip = ip;
    dhcp64_lease = lease;
    dhcp64_state = 3;
    return 0;
}

int dhcp64_bound_ip(u32 *ip, u32 *lease_left) {
    if (dhcp64_state != 3) return -1;
    if (ip) *ip = dhcp64_ip;
    if (lease_left) *lease_left = dhcp64_lease;
    return 0;
}
