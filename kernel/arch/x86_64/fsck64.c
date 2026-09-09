/* 40G: fsck — superblok + inode yuruyusu + tutarlilik bayraklari.
 * Bitler: 0x1 kotu superblok, 0x2 inode hatasi, 0x4 bag sayisi supheli.
 */
#include "arch/x86_64/longmode.h"

static u64 fsck64_nfiles = 0;
static u64 fsck64_nblocks = 0;

int fsck64_run(void) {
    u32 blocks = 0, inodes = 0, bsize = 0;
    int flags = 0;
    u32 ino;
    fsck64_nfiles = 0;
    fsck64_nblocks = 0;
    if (ext2_super(&blocks, &inodes, &bsize) != 0) return 0x1;
    if (!blocks || !inodes || bsize != 1024) return 0x1;
    if (blocks > 0x1000000UL || inodes > blocks * 4) return 0x1;
    for (ino = 1; ino <= inodes && ino < 4096; ino++) {
        struct ext2_inode in;
        int cls;
        if (ext2_read_inode(ino, &in) != 0) {
            flags |= 0x2;
            continue;
        }
        if (in.mode == 0) continue; /* bos inode */
        cls = in.mode & 0xF000;
        if (cls != 0x8000 && cls != 0x4000 && cls != 0xA000) {
            flags |= 0x2;
            continue;
        }
        if (in.links == 0) flags |= 0x4;
        if (in.size > 0x40000000UL) flags |= 0x2;
        fsck64_nfiles++;
        fsck64_nblocks += (in.size + 1023) / 1024;
    }
    return flags;
}

u64 fsck64_files(void) { return fsck64_nfiles; }
u64 fsck64_blocks(void) { return fsck64_nblocks; }
