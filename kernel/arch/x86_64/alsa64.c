/* 46B: ALSA PCM — durum makinesi + halka tampon + xrun. */
#include "arch/x86_64/longmode.h"

#define ALSA64_BUF 8192

static int alsa64_st = ALSA64_CLOSED;
static u32 alsa64_rate = 48000;
static int alsa64_ch = 2;
static int alsa64_fmt = 16;
static short alsa64_buf[ALSA64_BUF];
static u64 alsa64_head = 0;
static u64 alsa64_len = 0;
static u64 alsa64_hw_ptr = 0;

int alsa64_open(u32 rate, int ch, int fmt) {
    if (!rate || rate > 192000 || ch < 1 || ch > 8) return -1;
    if (fmt != 8 && fmt != 16 && fmt != 24 && fmt != 32) return -2;
    alsa64_rate = rate;
    alsa64_ch = ch;
    alsa64_fmt = fmt;
    alsa64_head = 0;
    alsa64_len = 0;
    alsa64_hw_ptr = 0;
    alsa64_st = ALSA64_SETUP;
    return 0;
}

int alsa64_hw_params(u32 rate, int ch, u32 *buf_frames, u32 *period) {
    u32 frames;
    if (!rate || ch < 1) return -1;
    /* 100ms tampon, 4 periyot */
    frames = rate / 10;
    if (buf_frames) *buf_frames = frames;
    if (period) *period = frames / 4;
    return 0;
}

int alsa64_prepare(void) {
    if (alsa64_st != ALSA64_SETUP && alsa64_st != ALSA64_XRUN) return -1;
    alsa64_head = 0;
    alsa64_len = 0;
    alsa64_st = ALSA64_PREPARED;
    return 0;
}

int alsa64_writei(const short *buf, u32 frames) {
    u64 need, i;
    if (alsa64_st != ALSA64_PREPARED && alsa64_st != ALSA64_RUNNING)
        return -1;
    if (!buf) return -1;
    need = (u64)frames * (u64)alsa64_ch;
    if (need > ALSA64_BUF - alsa64_len) {
        alsa64_st = ALSA64_XRUN; /* tasma */
        return -2;
    }
    for (i = 0; i < need; i++)
        alsa64_buf[(alsa64_head + alsa64_len + i) % ALSA64_BUF] = buf[i];
    alsa64_len += need;
    alsa64_st = ALSA64_RUNNING;
    /* Skeleton tuketim: yariyi donanim imleci ilerletir */
    {
        u64 consume = need / 2;
        alsa64_head = (alsa64_head + consume) % ALSA64_BUF;
        alsa64_len -= consume;
        alsa64_hw_ptr += consume / (u64)alsa64_ch;
    }
    return (int)frames;
}

u32 alsa64_avail(void) {
    u64 free = ALSA64_BUF - alsa64_len;
    return (u32)(free / (u64)alsa64_ch);
}

int alsa64_state(void) { return alsa64_st; }

int alsa64_close(void) {
    alsa64_st = ALSA64_CLOSED;
    return 0;
}
