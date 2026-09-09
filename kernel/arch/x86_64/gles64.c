/* 47D: OpenGL ES 3.2 — baglam + gorunum + derleyici iskeleti. */
#include "arch/x86_64/longmode.h"

#define GLES64_MAX_SHADER 32

struct gles64_shader {
    int used;
    int type; /* 0=vertex, 1=fragment, 2=compute */
    int compiled;
};

static struct gles64_shader gles64_shaders[GLES64_MAX_SHADER];
static u32 gles64_major = 0, gles64_minor = 0;
static u32 gles64_vp[4] = {0, 0, 0, 0};
static int gles64_ctx = 0;

int gles64_context(u32 major, u32 minor, u32 w, u32 h) {
    if (major < 2 || major > 3 || (major == 3 && minor > 2)) return -1;
    if (!w || !h || w > 4096 || h > 4096) return -1;
    gles64_major = major;
    gles64_minor = minor;
    gles64_vp[0] = 0;
    gles64_vp[1] = 0;
    gles64_vp[2] = w;
    gles64_vp[3] = h;
    gles64_ctx = 1;
    return 0;
}

int gles64_viewport(u32 x, u32 y, u32 w, u32 h) {
    if (!gles64_ctx) return -1;
    if (x + w > gles64_vp[2] || y + h > gles64_vp[3]) return -2;
    gles64_vp[0] = x;
    gles64_vp[1] = y;
    gles64_vp[2] = w;
    gles64_vp[3] = h;
    return 0;
}

/* Skeleton derleme: "#version" on Eki + en az bir noktali-virgul sart. */
int gles64_shader(int type, const char *src) {
    int i, semi = 0, bad = 1;
    if (!gles64_ctx || !src) return -1;
    if (type < 0 || type > 2) return -1;
    if (src[0] == '#' && src[1] == 'v') bad = 0;
    for (i = 0; src[i]; i++) {
        if (src[i] == ';') {
            semi = 1;
            break;
        }
    }
    if (bad || !semi) return -2;
    for (i = 0; i < GLES64_MAX_SHADER; i++) {
        if (!gles64_shaders[i].used) {
            gles64_shaders[i].used = 1;
            gles64_shaders[i].type = type;
            gles64_shaders[i].compiled = 1;
            return i;
        }
    }
    return -3;
}

int gles64_link(int vs, int fs) {
    if (vs < 0 || vs >= GLES64_MAX_SHADER || fs < 0 ||
        fs >= GLES64_MAX_SHADER)
        return -1;
    if (!gles64_shaders[vs].used || !gles64_shaders[fs].used) return -1;
    if (gles64_shaders[vs].type != 0 || gles64_shaders[fs].type != 1)
        return -2;
    if (!gles64_shaders[vs].compiled || !gles64_shaders[fs].compiled)
        return -3;
    return 0; /* program id yerine 0=ok (skeleton) */
}
