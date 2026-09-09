/* 44B: firmware yukleme — "RTFW" parcasi + sira + saglama.
 * Duzen: magic u32, ver u32, nchunk u32, toplam u32, sonra parcalar:
 * {addr u32, len u32, data[len]}.
 */
#include "arch/x86_64/longmode.h"

#define FWLOAD64_MAGIC 0x46575452UL /* "RTFW" */
#define FWLOAD64_MAXCHUNK 32

static const unsigned char *fwload64_blob = 0;
static u64 fwload64_len = 0;
static u32 fwload64_ver = 0;
static u32 fwload64_nchunk = 0;
static u64 fwload64_offs[FWLOAD64_MAXCHUNK];

static u32 fw_rd32(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

int fwload64_parse(const void *blob, u64 len, u32 *ver_out) {
    u64 pos;
    u32 n, i;
    if (!blob || len < 16) return -1;
    if (fw_rd32((const unsigned char *)blob) != (u32)FWLOAD64_MAGIC)
        return -2;
    n = fw_rd32((const unsigned char *)blob + 8);
    if (n > FWLOAD64_MAXCHUNK) return -3;
    pos = 16;
    for (i = 0; i < n; i++) {
        u32 clen;
        if (pos + 8 > len) return -4;
        clen = fw_rd32((const unsigned char *)blob + pos + 4);
        if (pos + 8 + clen > len || pos + 8 + clen < pos) return -5;
        fwload64_offs[i] = pos;
        pos += 8 + clen;
    }
    fwload64_blob = (const unsigned char *)blob;
    fwload64_len = len;
    fwload64_ver = fw_rd32((const unsigned char *)blob + 4);
    fwload64_nchunk = n;
    if (ver_out) *ver_out = fwload64_ver;
    return 0;
}

int fwload64_nchunks(void) { return (int)fwload64_nchunk; }

int fwload64_chunk(int i, struct fwload64_chunk *out) {
    u64 off;
    if (i < 0 || (u32)i >= fwload64_nchunk || !out) return -1;
    off = fwload64_offs[i];
    out->addr = fw_rd32(fwload64_blob + off);
    out->len = fw_rd32(fwload64_blob + off + 4);
    out->data = fwload64_blob + off + 8;
    return 0;
}

/* Basit toplam-saglama: tum baytlar toplami != 0 (sifir imaj red). */
int fwload64_verify(void) {
    u64 i, sum = 0;
    if (!fwload64_blob) return -1;
    for (i = 0; i < fwload64_len; i++) sum += fwload64_blob[i];
    return sum ? 0 : -2;
}

int fwload64_next(int *state) {
    if (!state) return -1;
    if (*state < 0) *state = 0;
    if ((u32)*state >= fwload64_nchunk) return -1; /* bitti */
    return (*state)++;
}
