/* 46D: PulseAudio server — istemci/akış + yazilim karistirma. */
#include "arch/x86_64/longmode.h"

#define PULSE64_MAX_CLIENTS 8
#define PULSE64_MAX_STREAMS 16
#define PULSE64_BUF 4096

struct pulse64_stream {
    int used;
    int client;
    u32 rate;
    int ch;
    int vol; /* 0..100 */
    short buf[PULSE64_BUF];
    int n; /* ornek sayisi (kanal-arasi) */
};

static int pulse64_clients[8];
static int pulse64_nclients = 0;
static struct pulse64_stream pulse64_streams[PULSE64_MAX_STREAMS];

int pulse64_client_add(const char *name) {
    (void)name;
    if (pulse64_nclients >= 8) return -1;
    pulse64_clients[pulse64_nclients] = pulse64_nclients;
    return pulse64_nclients++;
}

int pulse64_stream_open(int client, u32 rate, int ch) {
    int i;
    if (client < 0 || client >= pulse64_nclients) return -1;
    if (!rate || ch < 1 || ch > 2) return -1;
    for (i = 0; i < PULSE64_MAX_STREAMS; i++) {
        if (!pulse64_streams[i].used) {
            pulse64_streams[i].used = 1;
            pulse64_streams[i].client = client;
            pulse64_streams[i].rate = rate;
            pulse64_streams[i].ch = ch;
            pulse64_streams[i].vol = 100;
            pulse64_streams[i].n = 0;
            return i;
        }
    }
    return -2;
}

int pulse64_set_volume(int stream, int vol) {
    if (stream < 0 || stream >= PULSE64_MAX_STREAMS) return -1;
    if (!pulse64_streams[stream].used) return -1;
    if (vol < 0) vol = 0;
    if (vol > 150) vol = 150;
    pulse64_streams[stream].vol = vol;
    return 0;
}

int pulse64_write(int stream, const short *buf, int frames) {
    struct pulse64_stream *s;
    int i, space;
    if (stream < 0 || stream >= PULSE64_MAX_STREAMS) return -1;
    s = &pulse64_streams[stream];
    if (!s->used || !buf || frames <= 0) return -1;
    space = PULSE64_BUF - s->n;
    if ((int)(frames * s->ch) > space) return -2;
    for (i = 0; i < frames * s->ch; i++) s->buf[s->n++] = buf[i];
    return frames;
}

/* Tum akislari topla (ses siddeti + kirpma); cikarilan ornek sayisi. */
int pulse64_mix(short *out, int frames) {
    int i, f;
    if (!out || frames <= 0) return -1;
    for (f = 0; f < frames; f++) {
        int acc_l = 0, acc_r = 0;
        for (i = 0; i < PULSE64_MAX_STREAMS; i++) {
            struct pulse64_stream *s = &pulse64_streams[i];
            int v;
            if (!s->used || s->n < s->ch) continue;
            v = s->buf[0];
            acc_l += v * s->vol / 100;
            if (s->ch > 1)
                acc_r += s->buf[1] * s->vol / 100;
            else
                acc_r += v * s->vol / 100;
        }
        /* Akislari ilerlet (tuketilen ilk cerceve) */
        for (i = 0; i < PULSE64_MAX_STREAMS; i++) {
            struct pulse64_stream *s = &pulse64_streams[i];
            int k;
            if (!s->used || s->n < s->ch) continue;
            for (k = 0; k + s->ch < s->n; k++)
                s->buf[k] = s->buf[k + s->ch];
            s->n -= s->ch;
        }
        if (acc_l > 32767) acc_l = 32767;
        if (acc_l < -32768) acc_l = -32768;
        if (acc_r > 32767) acc_r = 32767;
        if (acc_r < -32768) acc_r = -32768;
        out[f * 2] = (short)acc_l;
        out[f * 2 + 1] = (short)acc_r;
    }
    return frames;
}
