/* 57E: replay sistem — kaydet/çal/oyna. */
#include "arch/x86_64/longmode.h"

#define REPLAY_MAX 1
#define REPLAY_FRAMES 1024

struct replay_entry {
    int used;
    char name[64];
    int quality;
    int frames;
    int data[REPLAY_FRAMES];
};

struct replay_state {
    int active;
    int recording;
    int playing;
    struct replay_entry entry;
};

static struct replay_state rs = {0};

static void replay_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int replay64_start(const char *name) {
    if (rs.active) return -1;
    if (!name) return -2;
    rs.active = 1;
    rs.recording = 0;
    rs.playing = 0;
    replay_str_copy(rs.entry.name, name, 64);
    rs.entry.used = 1;
    rs.entry.quality = 80;
    rs.entry.frames = 0;
    int i;
    for (i = 0; i < REPLAY_FRAMES; i++) rs.entry.data[i] = 0;
    return 0;
}

int replay64_stop(void) {
    if (!rs.active) return -1;
    rs.active = 0;
    rs.recording = 0;
    rs.playing = 0;
    return 0;
}

int replay64_is_recording(void) {
    return rs.recording ? 1 : 0;
}

int replay64_quality(int q) {
    if (!rs.active) return -1;
    if (q < 0 || q > 100) return -2;
    rs.entry.quality = q;
    return 0;
}

int replay64_get_frame_count(int *out) {
    if (!rs.active || !out) return -1;
    *out = rs.entry.frames;
    return 0;
}

int replay64_playback_start(const char *name) {
    if (!rs.active) return -1;
    if (!name) return -2;
    if (!str_eq(rs.entry.name, name)) return -3;
    if (rs.entry.frames == 0) return -4;
    rs.playing = 1;
    return 0;
}

int replay64_playback_stop(void) {
    if (!rs.active || !rs.playing) return -1;
    rs.playing = 0;
    return 0;
}

int replay64_record_start(void) {
    if (!rs.active) return -1;
    if (rs.recording) return -2;
    rs.recording = 1;
    rs.entry.frames = 0;
    return 0;
}

int replay64_record_stop(void) {
    if (!rs.active || !rs.recording) return -1;
    rs.recording = 0;
    return 0;
}

int replay64_record_frame(int frame) {
    if (!rs.active || !rs.recording) return -1;
    if (rs.entry.frames >= REPLAY_FRAMES) return -2;
    rs.entry.data[rs.entry.frames++] = frame;
    return 0;
}

int replay64_save(const char *name) {
    if (!rs.active) return -1;
    if (!name) return -2;
    replay_str_copy(rs.entry.name, name, 64);
    return 0;
}

int replay64_load(const char *name) {
    if (!rs.active) return -1;
    if (!name) return -2;
    if (!rs.entry.used) return -3;
    if (!str_eq(rs.entry.name, name)) return -4;
    return 0;
}
