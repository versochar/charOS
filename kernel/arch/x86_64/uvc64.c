/* 45C: USB video (UVC) — probe/taahhut + bant hesabi + format GUID. */
#include "arch/x86_64/longmode.h"

#define UVC64_YUY2 0
#define UVC64_MJPEG 1
#define UVC64_H264 2

static const unsigned char uvc64_guid_yuy2[16] = {
    'Y', 'U', 'Y', '2', 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71
};
static const unsigned char uvc64_guid_mjpeg[16] = {
    'M', 'J', 'P', 'G', 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71
};
static const unsigned char uvc64_guid_h264[16] = {
    'H', '2', '6', '4', 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71
};

static int uvc64_bpp(int fmt) {
    if (fmt == UVC64_YUY2) return 16;
    if (fmt == UVC64_MJPEG) return 12; /* tipik sikistirma sonrasi */
    if (fmt == UVC64_H264) return 6;
    return -1;
}

int uvc64_bandwidth(u32 w, u32 h, u32 fps, int fmt, u64 *out) {
    int bpp = uvc64_bpp(fmt);
    u64 bw;
    if (!w || !h || !fps || bpp < 0) return -1;
    if (w > 4096 || h > 4096 || fps > 120) return -2;
    bw = (u64)w * h * (u64)bpp / 8 * fps;
    if (out) *out = bw;
    return 0;
}

/* 0=kabul (bant USB2 480Mbps = 60MB/sn siniri icinde), <0 red. */
int uvc64_probe(u32 w, u32 h, u32 fps, int fmt, u64 *bw_out) {
    u64 bw;
    if (uvc64_bandwidth(w, h, fps, fmt, &bw) != 0) return -1;
    if (bw_out) *bw_out = bw;
    if (bw > 60000000ULL) return -2; /* USB2 asimi */
    return 0;
}

int uvc64_format_guid(int fmt, unsigned char guid[16]) {
    const unsigned char *src;
    int i;
    if (fmt == UVC64_YUY2)
        src = uvc64_guid_yuy2;
    else if (fmt == UVC64_MJPEG)
        src = uvc64_guid_mjpeg;
    else if (fmt == UVC64_H264)
        src = uvc64_guid_h264;
    else
        return -1;
    if (guid)
        for (i = 0; i < 16; i++) guid[i] = src[i];
    return 0;
}
