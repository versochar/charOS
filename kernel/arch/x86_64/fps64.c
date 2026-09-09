/* 57C: FPS overlay — kare hızı ölçüm + ekran üstü gösterim. */
#include "arch/x86_64/longmode.h"

struct fps_state {
    int active;
    u64 frames;
    u64 last_ms;
    u64 acc_ms;
    u64 fps;
    int overlay;
    int x;
    int y;
    int fmt; /* 0 = fps, 1 = ms */
};

static struct fps_state fps = {0};

int fps64_start(void) {
    if (fps.active) return -1;
    fps.active = 1;
    fps.frames = 0;
    fps.acc_ms = 0;
    fps.last_ms = (u64)-1;
    fps.fps = 0;
    fps.overlay = 1;
    fps.x = 10;
    fps.y = 10;
    fps.fmt = 0;
    return 0;
}

int fps64_stop(void) {
    if (!fps.active) return -1;
    fps.active = 0;
    return 0;
}

int fps64_tick(u64 now_ms) {
    if (!fps.active) return -1;
    if (fps.last_ms == (u64)-1) {
        fps.last_ms = now_ms;
        fps.frames = 0;
        fps.acc_ms = 0;
        return 0;
    }
    u64 delta = now_ms - fps.last_ms;
    fps.last_ms = now_ms;
    fps.acc_ms += delta;
    fps.frames++;
    if (fps.acc_ms >= 1000) {
        fps.fps = fps.frames * 1000 / fps.acc_ms;
        fps.frames = 0;
        fps.acc_ms = 0;
    }
    return 0;
}

int fps64_fps(u64 *out) {
    if (!fps.active || !out) return -1;
    *out = fps.fps;
    return 0;
}

int fps64_overlay(int on) {
    if (!fps.active) return -1;
    fps.overlay = on ? 1 : 0;
    return 0;
}

int fps64_pos(int x, int y) {
    if (!fps.active) return -1;
    if (x < 0 || y < 0) return -2;
    fps.x = x;
    fps.y = y;
    return 0;
}

int fps64_get_pos(int *x, int *y) {
    if (!fps.active || !x || !y) return -1;
    *x = fps.x;
    *y = fps.y;
    return 0;
}

int fps64_set_format(int fmt) {
    if (!fps.active) return -1;
    if (fmt < 0 || fmt > 1) return -2;
    fps.fmt = fmt;
    return 0;
}

int fps64_format(int *out) {
    if (!fps.active || !out) return -1;
    *out = fps.fmt;
    return 0;
}
