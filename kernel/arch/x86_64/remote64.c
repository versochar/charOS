/* 55G: remote desktop — el-sikisma + girdi yonlendirme + guncelleme. */
#include "arch/x86_64/longmode.h"

#define REMOTE64_MAX 8

struct remote64_session {
    int used;
    char client[48];
    u32 w, h;
    int authed;
    int seq;
};

static struct remote64_session remote64_tab[REMOTE64_MAX];

static void remote_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int remote64_hello(const char *client, u32 w, u32 h) {
    int i;
    if (!client || !w || !h || w > 4096 || h > 4096) return -1;
    for (i = 0; i < REMOTE64_MAX; i++) {
        if (!remote64_tab[i].used) {
            remote64_tab[i].used = 1;
            remote_str_copy(remote64_tab[i].client, client, 48);
            remote64_tab[i].w = w;
            remote64_tab[i].h = h;
            remote64_tab[i].authed = 0;
            remote64_tab[i].seq = 0;
            return i;
        }
    }
    return -2;
}

int remote64_handshake(int id, const char *token) {
    static const unsigned char magic[8] = {'R', 'E', 'M', 'O',
                                           'T', 'E', '1', '2'};
    int i;
    if (id < 0 || id >= REMOTE64_MAX || !remote64_tab[id].used)
        return -1;
    if (!token) return -1;
    /* Jeton: 8 bayt magic karsilastirma (ag sirasi) */
    for (i = 0; i < 8; i++) {
        if (!token[i]) return -2;
        if ((unsigned char)token[i] != magic[i]) return -3;
    }
    remote64_tab[id].authed = 1;
    return 0;
}

int remote64_input(int id, int type, int code, int value) {
    if (id < 0 || id >= REMOTE64_MAX || !remote64_tab[id].used)
        return -1;
    if (!remote64_tab[id].authed) return -2;
    (void)type;
    (void)code;
    (void)value;
    remote64_tab[id].seq++;
    return 0;
}

/* Guncelleme: tam-ekran tek dikdortgen (x,y,w,h sirali). */
int remote64_update(int id, int *rects, int max) {
    if (id < 0 || id >= REMOTE64_MAX || !remote64_tab[id].used)
        return -1;
    if (!remote64_tab[id].authed) return -2;
    if (!rects || max < 4) return -1;
    rects[0] = 0;
    rects[1] = 0;
    rects[2] = (int)remote64_tab[id].w;
    rects[3] = (int)remote64_tab[id].h;
    return 4;
}

int remote64_disconnect(int id) {
    if (id < 0 || id >= REMOTE64_MAX || !remote64_tab[id].used)
        return -1;
    remote64_tab[id].used = 0;
    return 0;
}
