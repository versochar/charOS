/* 50C: sshfs — baglanti + SFTP paketi + oznitelik cozumu. */
#include "arch/x86_64/longmode.h"

static int sshfs64_connected = 0;
static char sshfs64_host[64];
static char sshfs64_user[32];
static int sshfs64_port = 22;

static void ssh_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int sshfs64_connect(const char *host, const char *user, int port) {
    if (!host || !user) return -1;
    ssh_str_copy(sshfs64_host, host, 64);
    ssh_str_copy(sshfs64_user, user, 32);
    sshfs64_port = port > 0 ? port : 22;
    sshfs64_connected = 0; /* kimlik-dogrulama bekleniyor */
    return 0;
}

int sshfs64_auth(const char *password) {
    if (!password || !password[0]) return -1;
    sshfs64_connected = 1;
    return 0;
}

static void ssh_wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)((v >> 24) & 0xFF);
    p[1] = (unsigned char)((v >> 16) & 0xFF);
    p[2] = (unsigned char)((v >> 8) & 0xFF);
    p[3] = (unsigned char)(v & 0xFF);
}

/* SFTP paketi: uzunluk(4, kendisi haric) + tur(1) + id(4) + yuk. */
int sshfs64_packet(u32 type, u32 id, const void *payload, u64 len,
                   unsigned char *out, int max) {
    const unsigned char *p;
    u64 i;
    if (!sshfs64_connected) return -1;
    if (type > 255 || !out) return -1;
    if (!payload) len = 0;
    if (len + 9 > (u64)max) return -2;
    ssh_wr32(out + 0, (u32)(len + 5));
    out[4] = (unsigned char)type;
    ssh_wr32(out + 5, id);
    p = (const unsigned char *)payload;
    for (i = 0; i < len; i++) out[9 + i] = p[i];
    return (int)(len + 9);
}

/* SSH_FILEXFER_ATTR: bayrak(4) + [boyut(8)] + [uid/gid(4+4)] + [mod(4)].
 * Donus okunan bayt. */
int sshfs64_attr_parse(const unsigned char *p, int len, u64 *size,
                       u32 *mode) {
    u32 flags;
    int pos = 4;
    u64 sz = 0;
    u32 md = 0;
    int i;
    if (!p || len < 4) return -1;
    flags = ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) |
            p[3];
    if ((flags & 0x1) && pos + 8 <= len) {
        for (i = 0; i < 8; i++) sz = (sz << 8) | p[pos + i];
        pos += 8;
    }
    if ((flags & 0x2) && pos + 8 <= len) pos += 8; /* uid+gid atla */
    if ((flags & 0x4) && pos + 4 <= len) {
        md = ((u32)p[pos] << 24) | ((u32)p[pos + 1] << 16) |
             ((u32)p[pos + 2] << 8) | p[pos + 3];
        pos += 4;
    }
    if (size) *size = sz;
    if (mode) *mode = md;
    return pos;
}
