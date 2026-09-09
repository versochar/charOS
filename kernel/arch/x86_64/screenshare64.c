/* 55F: screen sharing — yayin + kare uretimi (test deseni). */
#include "arch/x86_64/longmode.h"

#define SCREENSHARE64_MAX 4

struct screenshare64_stream {
    int used;
    int output;
    u32 fps;
    u64 frames;
    int subs;
};

static struct screenshare64_stream screenshare64_tab[SCREENSHARE64_MAX];

int screenshare64_start(int output, u32 fps) {
    int i;
    if (output < 0 || !fps || fps > 120) return -1;
    for (i = 0; i < SCREENSHARE64_MAX; i++) {
        if (!screenshare64_tab[i].used) {
            screenshare64_tab[i].used = 1;
            screenshare64_tab[i].output = output;
            screenshare64_tab[i].fps = fps;
            screenshare64_tab[i].frames = 0;
            screenshare64_tab[i].subs = 1;
            return i;
        }
    }
    return -2;
}

/* Kare: capraz gecis deseni (cikti boyu sinirli, kalan kirpilir). */
int screenshare64_frame(int id, unsigned char *buf, u64 max) {
    u64 i, n;
    if (id < 0 || id >= SCREENSHARE64_MAX || !screenshare64_tab[id].used)
        return -1;
    if (!buf || !max) return -1;
    n = max < 256 ? max : 256;
    for (i = 0; i < n; i++)
        buf[i] = (unsigned char)((i + screenshare64_tab[id].frames) & 0xFF);
    screenshare64_tab[id].frames++;
    return (int)n;
}

int screenshare64_stop(int id) {
    if (id < 0 || id >= SCREENSHARE64_MAX || !screenshare64_tab[id].used)
        return -1;
    screenshare64_tab[id].used = 0;
    return 0;
}

int screenshare64_subscribers(int id) {
    if (id < 0 || id >= SCREENSHARE64_MAX || !screenshare64_tab[id].used)
        return -1;
    return screenshare64_tab[id].subs;
}
