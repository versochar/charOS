/* 40A: ext2 driver — 1K blok, ilk 12 direkt blok, salt-okuma agirlikli.
 * Backend: caller'in rd/wr fonksiyonlari (RAM imaji testte).
 */
#include "arch/x86_64/longmode.h"

#define EXT2_BSIZE 1024
#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INO 2

static int (*ext2_rd)(u32 blk, void *buf) = 0;
static int (*ext2_wr)(u32 blk, const void *buf) = 0;
static int ext2_mounted = 0;
static u32 ext2_blocks = 0;
static u32 ext2_inodes = 0;
static u32 ext2_inodes_per_group = 0;
static u32 ext2_inode_size = 128;
static u32 ext2_first_data = 1;

static u32 rd16(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8);
}

static u32 rd32(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static int ext2_rd_blk(u32 blk, unsigned char *buf) {
    if (!ext2_rd) return -1;
    return ext2_rd(blk, buf);
}

int ext2_attach(int (*rd)(u32 blk, void *buf),
                int (*wr)(u32 blk, const void *buf)) {
    if (!rd) return -1;
    ext2_rd = rd;
    ext2_wr = wr;
    ext2_mounted = 0;
    return 0;
}

int ext2_mount(void) {
    unsigned char sb[EXT2_BSIZE];
    u32 logsz;
    if (!ext2_rd) return -1;
    if (ext2_rd_blk(1, sb) != 0) return -2;
    if (rd16(sb + 56) != EXT2_MAGIC) return -3;
    logsz = rd32(sb + 24);
    if (logsz != 0) return -4; /* skeleton: yalniz 1K */
    ext2_inodes = rd32(sb + 0);
    ext2_blocks = rd32(sb + 4);
    ext2_first_data = rd32(sb + 20);
    ext2_inodes_per_group = rd32(sb + 40);
    {
        u32 isz = rd16(sb + 88);
        if (isz >= 128 && isz <= 512) ext2_inode_size = isz;
    }
    if (!ext2_inodes || !ext2_blocks || !ext2_inodes_per_group) return -5;
    ext2_mounted = 1;
    return 0;
}

int ext2_super(u32 *blocks, u32 *inodes, u32 *bsize) {
    if (!ext2_mounted) return -1;
    if (blocks) *blocks = ext2_blocks;
    if (inodes) *inodes = ext2_inodes;
    if (bsize) *bsize = EXT2_BSIZE;
    return 0;
}

/* Grup tanimlayici: grup 0, blok 2 (1K blokta). */
static int ext2_group(u32 *inode_table) {
    unsigned char gd[EXT2_BSIZE];
    if (ext2_rd_blk(2, gd) != 0) return -1;
    if (inode_table) *inode_table = rd32(gd + 8);
    return 0;
}

int ext2_read_inode(u32 ino, struct ext2_inode *out) {
    unsigned char blk[EXT2_BSIZE];
    u32 table, idx, off;
    const unsigned char *p;
    int i;
    if (!ext2_mounted || !out || !ino || ino > ext2_inodes) return -1;
    if (ext2_group(&table) != 0) return -2;
    idx = (ino - 1) % ext2_inodes_per_group;
    off = idx * ext2_inode_size;
    if (ext2_rd_blk(table + off / EXT2_BSIZE, blk) != 0) return -3;
    p = blk + (off % EXT2_BSIZE);
    out->mode = (u16)rd16(p + 0);
    out->uid = (u16)rd16(p + 2);
    out->size = rd32(p + 4);
    out->gid = (u16)rd16(p + 24);
    out->links = (u16)rd16(p + 26);
    for (i = 0; i < 15; i++)
        out->blocks[i] = (i < 12) ? rd32(p + 40 + i * 4) : 0;
    return 0;
}

/* Yalniz ilk 12 direkt blok (skeleton siniri: 12KB dosya). */
int ext2_read_file(u32 ino, u64 off, void *buf, u64 len) {
    struct ext2_inode in;
    unsigned char blk[EXT2_BSIZE];
    unsigned char *d = (unsigned char *)buf;
    u64 done = 0;
    if (!buf || ext2_read_inode(ino, &in) != 0) return -1;
    if ((in.mode & 0xF000) != 0x8000 && (in.mode & 0xF000) != 0x4000 &&
        (in.mode & 0xF000) != 0xA000)
        return -2;
    if (off >= in.size) return 0;
    if (off + len > in.size) len = in.size - off;
    while (done < len) {
        u64 pos = off + done;
        u32 bidx = (u32)(pos / EXT2_BSIZE);
        u32 boff = (u32)(pos % EXT2_BSIZE);
        u64 chunk;
        if (bidx >= 12) break; /* dolayli blok 41x'te */
        if (!in.blocks[bidx]) break;
        if (ext2_rd_blk(in.blocks[bidx], blk) != 0) break;
        chunk = EXT2_BSIZE - boff;
        if (chunk > len - done) chunk = len - done;
        for (u64 i = 0; i < chunk; i++) d[done + i] = blk[boff + i];
        done += chunk;
    }
    return (int)done;
}

int ext2_lookup(u32 dir_ino, const char *name, u32 *ino_out) {
    struct ext2_inode in;
    unsigned char blk[EXT2_BSIZE];
    u64 pos = 0;
    u64 nl = 0;
    if (!name || ext2_read_inode(dir_ino, &in) != 0) return -1;
    if ((in.mode & 0xF000) != 0x4000) return -2;
    while (name[nl]) nl++;
    while (pos + 8 <= in.size) {
        u32 bidx = (u32)(pos / EXT2_BSIZE);
        u32 boff = (u32)(pos % EXT2_BSIZE);
        u32 e_ino, e_len, e_namelen, i;
        int match;
        if (bidx >= 12 || !in.blocks[bidx]) break;
        if (ext2_rd_blk(in.blocks[bidx], blk) != 0) break;
        if (boff + 8 > EXT2_BSIZE) break;
        e_ino = rd32(blk + boff);
        e_len = rd16(blk + boff + 4);
        e_namelen = blk[boff + 6];
        if (!e_len || boff + e_len > EXT2_BSIZE) break;
        match = 1;
        if ((u64)e_namelen != nl)
            match = 0;
        else {
            for (i = 0; i < e_namelen; i++) {
                if (blk[boff + 8 + i] != (unsigned char)name[i]) {
                    match = 0;
                    break;
                }
            }
        }
        if (match && e_ino) {
            if (ino_out) *ino_out = e_ino;
            return 0;
        }
        pos += e_len;
    }
    return -3; /* bulunamadi */
}
