/* 46J: media player — calma listesi + tasima durumu + konum takibi. */
#include "arch/x86_64/longmode.h"

#define MEDIA64_MAX_TRACKS 32

struct media64_track {
    int used;
    char path[96];
    u64 duration_ms;
};

static struct media64_track media64_list[MEDIA64_MAX_TRACKS];
static int media64_n = 0;
static int media64_cur = -1;
static int media64_st = MEDIA64_STOPPED;
static u64 media64_base_ms = 0; /* calma baslangic ofseti */

static void media_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int media64_playlist_add(const char *path, u64 duration_ms) {
    int i;
    if (!path) return -1;
    for (i = 0; i < MEDIA64_MAX_TRACKS; i++) {
        if (!media64_list[i].used) {
            media64_list[i].used = 1;
            media_str_copy(media64_list[i].path, path, 96);
            media64_list[i].duration_ms = duration_ms;
            media64_n++;
            if (media64_cur < 0) media64_cur = i;
            return 0;
        }
    }
    return -2;
}

static struct media64_track *media64_track(void) {
    if (media64_cur < 0 || media64_cur >= MEDIA64_MAX_TRACKS) return 0;
    if (!media64_list[media64_cur].used) return 0;
    return &media64_list[media64_cur];
}

int media64_play(void) {
    if (!media64_track()) return -1;
    media64_st = MEDIA64_PLAYING;
    return 0;
}

int media64_pause(void) {
    if (media64_st != MEDIA64_PLAYING) return -1;
    media64_st = MEDIA64_PAUSED;
    return 0;
}

int media64_stop(void) {
    media64_st = MEDIA64_STOPPED;
    media64_base_ms = 0;
    return 0;
}

static int media64_first_used(void) {
    int i;
    for (i = 0; i < MEDIA64_MAX_TRACKS; i++)
        if (media64_list[i].used) return i;
    return -1;
}

int media64_next(void) {
    int i;
    if (media64_cur < 0) return -1;
    for (i = media64_cur + 1; i < MEDIA64_MAX_TRACKS; i++) {
        if (media64_list[i].used) {
            media64_cur = i;
            media64_base_ms = 0;
            return 0;
        }
    }
    /* Basa sar */
    i = media64_first_used();
    if (i < 0) return -2;
    media64_cur = i;
    media64_base_ms = 0;
    return 0;
}

int media64_prev(void) {
    int i;
    if (media64_cur < 0) return -1;
    for (i = media64_cur - 1; i >= 0; i--) {
        if (media64_list[i].used) {
            media64_cur = i;
            media64_base_ms = 0;
            return 0;
        }
    }
    return -2; /* bastayiz */
}

int media64_seek(u64 ms) {
    struct media64_track *t = media64_track();
    if (!t) return -1;
    if (ms > t->duration_ms) ms = t->duration_ms;
    media64_base_ms = ms;
    return 0;
}

u64 media64_position(u64 elapsed_ms) {
    struct media64_track *t = media64_track();
    u64 pos;
    if (!t || media64_st != MEDIA64_PLAYING) return media64_base_ms;
    pos = media64_base_ms + elapsed_ms;
    if (pos > t->duration_ms) pos = t->duration_ms;
    return pos;
}

int media64_state(void) { return media64_st; }
