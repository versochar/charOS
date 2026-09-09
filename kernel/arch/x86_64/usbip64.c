/* 45F: USB/IP — gonderim/geri-donus basliklari (ag sirasi, BE).
 * Baslik: ver(2) + komut(2) + sira(4) + cihaz(4) + yon(4) + ep(4) [+ URB].
 */
#include "arch/x86_64/longmode.h"

#define USBIP64_HDR 24

static void usbip_wr16(unsigned char *p, u32 v) {
    p[0] = (unsigned char)((v >> 8) & 0xFF);
    p[1] = (unsigned char)(v & 0xFF);
}

static void usbip_wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)((v >> 24) & 0xFF);
    p[1] = (unsigned char)((v >> 16) & 0xFF);
    p[2] = (unsigned char)((v >> 8) & 0xFF);
    p[3] = (unsigned char)(v & 0xFF);
}

static u32 usbip_rd32(const unsigned char *p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) |
           (u32)p[3];
}

int usbip64_build_submit(u32 seq, u32 devid, u8 ep, u32 flags,
                         unsigned char *out, int max) {
    if (!out || max < USBIP64_HDR + 16) return -1;
    usbip_wr16(out + 0, 0x0111); /* surum */
    usbip_wr16(out + 2, USBIP64_SUBMIT);
    usbip_wr32(out + 4, seq);
    usbip_wr32(out + 8, devid);
    usbip_wr32(out + 12, 0); /* giris yonu */
    usbip_wr32(out + 16, ep);
    usbip_wr32(out + 20, 0);              /* transfer bayraklari */
    usbip_wr32(out + 24, flags);          /* URB bayraklari */
    usbip_wr32(out + 28, 0);              /* tampon boyu */
    usbip_wr32(out + 32, 0);              /* baslangic cercevesi */
    usbip_wr32(out + 36, 0);              /* cikti sayisi */
    return USBIP64_HDR + 16;
}

int usbip64_build_unlink(u32 seq, u32 unseq, unsigned char *out, int max) {
    if (!out || max < USBIP64_HDR) return -1;
    usbip_wr16(out + 0, 0x0111);
    usbip_wr16(out + 2, USBIP64_UNLINK);
    usbip_wr32(out + 4, seq);
    usbip_wr32(out + 8, 0);
    usbip_wr32(out + 12, 0);
    usbip_wr32(out + 16, 0);
    usbip_wr32(out + 20, unseq);
    return USBIP64_HDR;
}

int usbip64_parse_ret(const unsigned char *msg, int len, u32 *seq,
                      int *status) {
    u32 ver, cmd;
    if (!msg || len < USBIP64_HDR) return -1;
    ver = ((u32)msg[0] << 8) | msg[1];
    cmd = ((u32)msg[2] << 8) | msg[3];
    if (ver != 0x0111) return -2;
    if (cmd != USBIP64_RET_SUBMIT && cmd != 0x4000 + USBIP64_UNLINK)
        return -3;
    if (seq) *seq = usbip_rd32(msg + 4);
    if (status) *status = (int)usbip_rd32(msg + 8);
    return 0;
}
