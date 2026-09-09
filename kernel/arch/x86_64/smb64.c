/* 50F: SMB istemci — lehce + agac + arka-uc okuma/yazma. */
#include "arch/x86_64/longmode.h"

#define SMB64_MAX_DIALECT 0x0311UL

static u32 smb64_dialect = 0;
static char smb64_share[64];
static int smb64_tid = -1;

static int (*smb64_rd)(u64 fh, u64 off, void *buf, u64 len) = 0;
static int (*smb64_wr)(u64 fh, u64 off, const void *buf, u64 len) = 0;

void smb64_set_backend(int (*rd)(u64, u64, void *, u64),
                       int (*wr)(u64, u64, const void *, u64)) {
    smb64_rd = rd;
    smb64_wr = wr;
}

static void smb_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

/* Desteklenen en yuksek lehceyi sec (<= 3.1.1). */
int smb64_negotiate(const u32 *dialects, int n) {
    int i;
    u32 best = 0;
    if (!dialects || n <= 0) return -1;
    for (i = 0; i < n; i++) {
        if (dialects[i] > SMB64_MAX_DIALECT) continue;
        if (dialects[i] > best) best = dialects[i];
    }
    if (!best) return -2;
    smb64_dialect = best;
    return 0;
}

int smb64_tree(const char *share) {
    if (!smb64_dialect || !share) return -1;
    smb_str_copy(smb64_share, share, 64);
    smb64_tid = 1;
    return smb64_tid;
}

int smb64_read(u64 fh, u64 off, void *buf, u64 len) {
    if (smb64_tid < 0) return -1;
    if (!smb64_rd) return -2;
    return smb64_rd(fh, off, buf, len);
}

int smb64_write(u64 fh, u64 off, const void *buf, u64 len) {
    if (smb64_tid < 0) return -1;
    if (!smb64_wr) return -2;
    return smb64_wr(fh, off, buf, len);
}
