/* 50D: NFS istemci — baglama + RPC basligi + arka-uc okuma/yazma. */
#include "arch/x86_64/longmode.h"

#define NFS64_PROG 100003
#define NFS64_VERS 3
#define NFS64_MOUNTED 1

static int nfsclient64_mounted = 0;
static char nfsclient64_host[64];
static char nfsclient64_path[64];

static int (*nfsclient64_rd)(u64 fh, u64 off, void *buf, u64 len) = 0;
static int (*nfsclient64_wr)(u64 fh, u64 off, const void *buf, u64 len) = 0;

void nfsclient64_set_backend(int (*rd)(u64, u64, void *, u64),
                             int (*wr)(u64, u64, const void *, u64)) {
    nfsclient64_rd = rd;
    nfsclient64_wr = wr;
}

static void nfs_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int nfsclient64_mount(const char *host, const char *path) {
    if (!host || !path || path[0] != '/') return -1;
    nfs_str_copy(nfsclient64_host, host, 64);
    nfs_str_copy(nfsclient64_path, path, 64);
    nfsclient64_mounted = NFS64_MOUNTED;
    return 0;
}

/* ONC-RPC cagri basligi (BE): xid + msgtype=0 + rpcvers + prog + vers +
 * proc + auth-null(8B). Donus boy. */
int nfsclient64_rpc(u32 prog, u32 vers, u32 proc, u32 xid,
                    unsigned char *out, int max) {
    int pos = 0;
    u32 fields[8];
    int i;
    (void)prog;
    (void)vers;
    if (!out || max < 40) return -1;
    fields[0] = xid;
    fields[1] = 0;          /* CALL */
    fields[2] = 2;          /* RPCv2 */
    fields[3] = NFS64_PROG; /* program her zaman NFS (skeleton) */
    fields[4] = NFS64_VERS;
    fields[5] = proc;
    fields[6] = 0; /* auth flavor */
    fields[7] = 0; /* auth boy */
    for (i = 0; i < 8; i++) {
        out[pos++] = (unsigned char)((fields[i] >> 24) & 0xFF);
        out[pos++] = (unsigned char)((fields[i] >> 16) & 0xFF);
        out[pos++] = (unsigned char)((fields[i] >> 8) & 0xFF);
        out[pos++] = (unsigned char)(fields[i] & 0xFF);
    }
    /* auth govde (8B sifir) */
    for (i = 0; i < 8; i++) out[pos++] = 0;
    return pos;
}

int nfsclient64_read(u64 fh, u64 off, void *buf, u64 len) {
    if (nfsclient64_mounted != NFS64_MOUNTED) return -1;
    if (!nfsclient64_rd) return -2;
    return nfsclient64_rd(fh, off, buf, len);
}

int nfsclient64_write(u64 fh, u64 off, const void *buf, u64 len) {
    if (nfsclient64_mounted != NFS64_MOUNTED) return -1;
    if (!nfsclient64_wr) return -2;
    return nfsclient64_wr(fh, off, buf, len);
}
