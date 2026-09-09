/* 41A: .char paket formati — baslik + dosya listesi sinir denetimi.
 * Duzen (LE): magic u32, name[32], ver[16], arch[8], nfiles u32,
 * sonra nfiles x {path[64], offset u64, size u64, hash u64}.
 */
#include "arch/x86_64/longmode.h"

#define CHARPKG64_NAMELEN 32
#define CHARPKG64_VERLEN 16
#define CHARPKG64_ARCHLEN 8
#define CHARPKG64_PATHLEN 64
#define CHARPKG64_HDRLEN (4 + 32 + 16 + 8 + 4)
#define CHARPKG64_ENTRYLEN (64 + 8 + 8 + 8)

static u32 cp_rd32(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static u64 cp_rd64(const unsigned char *p) {
    u64 v = 0;
    int i;
    for (i = 0; i < 8; i++) v |= (u64)p[i] << (i * 8);
    return v;
}

static void cp_copy(char *d, const unsigned char *s, int n) {
    int i;
    for (i = 0; i < n; i++) d[i] = (char)s[i];
    d[n - 1] = 0;
}

int charpkg64_parse(const void *buf, u64 len, struct charpkg64_hdr *hdr,
                    struct charpkg64_file *files, int max, int *count_out) {
    const unsigned char *b = (const unsigned char *)buf;
    u32 nfiles;
    u64 need;
    int i;
    if (!b || len < CHARPKG64_HDRLEN) return -1;
    if (cp_rd32(b) != (u32)CHARPKG64_MAGIC) return -2;
    nfiles = cp_rd32(b + 4 + 32 + 16 + 8);
    if (nfiles > 1024) return -3;
    need = CHARPKG64_HDRLEN + (u64)nfiles * CHARPKG64_ENTRYLEN;
    if (need > len) return -4;
    if (hdr) {
        hdr->magic = cp_rd32(b);
        cp_copy(hdr->name, b + 4, CHARPKG64_NAMELEN);
        cp_copy(hdr->ver, b + 4 + 32, CHARPKG64_VERLEN);
        cp_copy(hdr->arch, b + 4 + 32 + 16, CHARPKG64_ARCHLEN);
        hdr->nfiles = nfiles;
    }
    if (files && max > 0) {
        int n = nfiles < (u32)max ? (int)nfiles : max;
        for (i = 0; i < n; i++) {
            const unsigned char *e = b + CHARPKG64_HDRLEN +
                                     (u64)i * CHARPKG64_ENTRYLEN;
            cp_copy(files[i].path, e, CHARPKG64_PATHLEN);
            files[i].offset = cp_rd64(e + 64);
            files[i].size = cp_rd64(e + 72);
            files[i].hash = cp_rd64(e + 80);
            /* Veri araligi paket icinde mi? (baslik+liste sonrasi) */
            if (files[i].offset + files[i].size < files[i].offset ||
                files[i].offset + files[i].size > len)
                return -5;
        }
        if (count_out) *count_out = n;
        if ((u32)max < nfiles) return -6; /* tampon tasti */
    } else if (count_out) {
        *count_out = (int)nfiles;
    }
    return 0;
}

static int cp_path_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int charpkg64_find(const struct charpkg64_file *files, int n,
                   const char *path, u64 *off, u64 *size) {
    int i;
    if (!files || n <= 0 || !path) return -1;
    for (i = 0; i < n; i++) {
        if (cp_path_eq(files[i].path, path)) {
            if (off) *off = files[i].offset;
            if (size) *size = files[i].size;
            return 0;
        }
    }
    return -2;
}
