/* 40J: ext2_64/journal64/fsync64/xattr64/acl64/quota64/fsck64/snap64/
 *      fuse64 host testi + kiyas (benchmark).
 * Calistirma: make test-fs64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "arch/x86_64/longmode.h"

#define IMG_BLOCKS 64
#define BSZ 1024

static unsigned char img[IMG_BLOCKS][BSZ];

static int img_rd(u32 blk, void *buf) {
    if (blk >= IMG_BLOCKS || !buf) return -1;
    memcpy(buf, img[blk], BSZ);
    return 0;
}

static int img_wr(u32 blk, const void *buf) {
    if (blk >= IMG_BLOCKS || !buf) return -1;
    memcpy(img[blk], buf, BSZ);
    return 0;
}

static void wr16(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void wr32(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static const char *filedata = "charOS ext2 test icerigi 40A";

static void build_img(void) {
    unsigned char *sb = img[1];
    unsigned char *gd = img[2];
    unsigned char *it;
    unsigned char *root = img[7];
    memset(img, 0, sizeof(img));
    wr32(sb + 0, 16);   /* inodes */
    wr32(sb + 4, 64);   /* blocks */
    wr32(sb + 20, 1);   /* first data */
    wr32(sb + 24, 0);   /* 1K */
    wr32(sb + 32, 64);  /* per group */
    wr32(sb + 40, 16);
    wr16(sb + 56, 0xEF53);
    wr16(sb + 88, 128);
    wr32(gd + 8, 5);    /* inode table blok 5 */
    it = img[5];
    /* inode 2: kok dizin */
    wr16(it + 128 + 0, 0x41ED);
    wr32(it + 128 + 4, 1024);
    wr16(it + 128 + 26, 2);
    wr32(it + 128 + 40, 7);
    /* inode 3: dosya */
    wr16(it + 256 + 0, 0x81A4);
    wr32(it + 256 + 4, (u32)strlen(filedata));
    wr16(it + 256 + 26, 1);
    wr32(it + 256 + 40, 8);
    /* kok dizin girdileri */
    wr32(root + 0, 2);
    wr16(root + 4, 12);
    root[6] = 1;
    root[7] = 2;
    root[8] = '.';
    wr32(root + 12, 2);
    wr16(root + 16, 12);
    root[18] = 2;
    root[19] = 2;
    root[20] = '.';
    root[21] = '.';
    wr32(root + 24, 3);
    wr16(root + 28, 1000);
    root[30] = 5;
    root[31] = 1;
    memcpy(root + 32, "hello", 5);
    memcpy(img[8], filedata, strlen(filedata));
}

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

/* 40I RAM arka ucu */
static int fuse_lookup(const char *path, u64 *ino_out) {
    if (!strcmp(path, "/ram")) {
        if (ino_out) *ino_out = 42;
        return 0;
    }
    return -1;
}

static int fuse_read(u64 ino, u64 off, void *buf, u64 len) {
    const char *s = "fuse-ram";
    u64 sl = strlen(s), i;
    if (ino != 42 || !buf) return -1;
    for (i = 0; i < len && off + i < sl; i++)
        ((unsigned char *)buf)[i] = (unsigned char)s[off + i];
    return (int)i;
}

static int fuse_write(u64 ino, u64 off, const void *buf, u64 len) {
    (void)ino;
    (void)off;
    (void)buf;
    return (int)len;
}

static int fuse_getattr(u64 ino, int *type, u64 *size) {
    if (ino != 42) return -1;
    if (type) *type = VFS64_FILE;
    if (size) *size = 8;
    return 0;
}

static const struct fuse64_ops ram_ops = {
    fuse_lookup, fuse_read, fuse_write, 0, fuse_getattr
};

int main(void) {
    u32 b = 0, n = 0, bs = 0, ino = 0;
    struct ext2_inode in;
    char buf[128];
    unsigned char blk[1024];
    int i, rc;

    build_img();
    CHECK(ext2_attach(img_rd, img_wr) == 0, "40A attach");
    CHECK(ext2_mount() == 0, "40A mount");
    CHECK(ext2_super(&b, &n, &bs) == 0 && b == 64 && n == 16 && bs == 1024,
          "40A super");
    CHECK(ext2_read_inode(2, &in) == 0 && (in.mode & 0xF000) == 0x4000,
          "40A kok inode");
    CHECK(ext2_read_inode(3, &in) == 0 && in.size == strlen(filedata),
          "40A dosya inode");
    CHECK(ext2_lookup(2, "hello", &ino) == 0 && ino == 3, "40A lookup");
    CHECK(ext2_lookup(2, "yok", 0) != 0, "40A yok red");
    memset(buf, 0, sizeof(buf));
    rc = ext2_read_file(3, 0, buf, sizeof(buf));
    CHECK(rc == (int)strlen(filedata) && !strcmp(buf, filedata),
          "40A dosya oku");
    CHECK(ext2_read_file(3, 1000, buf, 10) == 0, "40A EOF");

    journal64_set_backend(img_wr);
    fsync64_set_backend(img_wr);
    snap64_set_backend(img_rd);
    memset(blk, 0x5A, sizeof(blk));
    CHECK(journal64_begin() == 0, "40B begin");
    CHECK(journal64_write(20, blk) == 0, "40B stage");
    CHECK(journal64_commit() == 1, "40B commit seq");
    CHECK(img[20][0] == 0x5A && img[20][1023] == 0x5A, "40B uygulandi");
    CHECK(journal64_seq() == 1, "40B seq");
    CHECK(journal64_begin() == 0, "40B begin2");
    memset(blk, 0xA5, sizeof(blk));
    CHECK(journal64_write(20, blk) == 0, "40B stage2");
    CHECK(journal64_abort() == 0, "40B abort");
    CHECK(img[20][0] == 0x5A, "40B abort yazmadi");
    CHECK(journal64_recover() >= 1, "40B recover");

    memset(blk, 0x33, sizeof(blk));
    CHECK(fsync64_mark_dirty(21, blk) == 0, "40C kirli");
    CHECK(fsync64_sync() == 1, "40C sync");
    CHECK(img[21][500] == 0x33, "40C uygulandi");
    CHECK(fsync64_barrier() >= 1, "40C bariyer");
    CHECK(fsync64_sync() == 0, "40C temiz sync");

    CHECK(xattr64_set(3, "user.test", "deger", 5) == 0, "40D kur");
    {
        char v[16];
        u64 l = sizeof(v);
        CHECK(xattr64_get(3, "user.test", v, &l) == 0 && l == 5 &&
              !memcmp(v, "deger", 5), "40D oku");
    }
    {
        char l[64];
        CHECK(xattr64_list(3, l, sizeof(l)) > 0, "40D liste");
    }
    CHECK(xattr64_remove(3, "user.test") == 0, "40D sil");
    CHECK(xattr64_get(3, "user.test", 0, 0) != 0, "40D sil sonrasi");

    CHECK(acl64_set(3, ACL64_USER_OBJ, 0, 6) == 0, "40E sahip");
    CHECK(acl64_set(3, ACL64_GROUP_OBJ, 0, 4) == 0, "40E grup");
    CHECK(acl64_set(3, ACL64_OTHER, 0, 4) == 0, "40E diger");
    CHECK(acl64_check(3, 0, 0, 6) == 1, "40E kok okur");
    CHECK(acl64_set(3, ACL64_USER, 1000, 7) == 0, "40E adli");
    CHECK(acl64_check(3, 1000, 100, 7) == 1, "40E adli tam");
    CHECK(acl64_set(3, ACL64_MASK, 0, 4) == 0, "40E maske");
    CHECK(acl64_check(3, 1000, 100, 7) == 0, "40E maske kirpar");
    CHECK(acl64_check(3, 1000, 100, 4) == 1, "40E maske ici");

    CHECK(quota64_set_limit(1000, 100, 10) == 0, "40F limit");
    CHECK(quota64_charge(1000, 50, 5) == 0, "40F tuket");
    CHECK(quota64_check(1000) == 0, "40F limit ici");
    CHECK(quota64_charge(1000, 60, 0) != 0, "40F asim red");
    CHECK(quota64_release(1000, 50, 5) == 0, "40F iade");
    CHECK(quota64_charge(1000, 60, 0) == 0, "40F iade sonrasi");

    {
        int fl = fsck64_run();
        CHECK(fl == 0, "40G temiz");
        CHECK(fsck64_files() >= 2, "40G dosya sayisi");
    }

    {
        int sid = snap64_create();
        unsigned char rb[1024], nb[1024];
        CHECK(sid >= 0, "40H olustur");
        memset(nb, 0x77, sizeof(nb));
        CHECK(snap64_write(sid, 8, img[8]) == 0, "40H dondur");
        memcpy(img[8], nb, sizeof(nb)); /* canli degisiklik */
        CHECK(snap64_read(sid, 8, rb) == 0 &&
              !memcmp(rb, filedata, strlen(filedata)), "40H eski icerik");
        CHECK(snap64_delete(sid) == 0, "40H sil");
        memcpy(img[8], filedata, strlen(filedata)); /* imaji onar */
    }

    {
        int h = fuse64_mount(&ram_ops);
        u64 fi = 0;
        int ty = 0;
        u64 sz = 0;
        char fb[16];
        CHECK(h >= 0, "40I bagla");
        CHECK(fuse64_lookup(h, "/ram", &fi) == 0 && fi == 42, "40I lookup");
        CHECK(fuse64_lookup(h, "/yok", 0) != 0, "40I yok red");
        CHECK(fuse64_getattr(h, 42, &ty, &sz) == 0 && ty == 1 && sz == 8,
              "40I getattr");
        memset(fb, 0, sizeof(fb));
        CHECK(fuse64_read(h, 42, 0, fb, sizeof(fb)) == 8 &&
              !strcmp(fb, "fuse-ram"), "40I read");
        CHECK(fuse64_lookup(99, "/ram", 0) != 0, "40I handle red");
    }

    /* 40J kiyas: 2000x sira okuma */
    {
        clock_t t0 = clock();
        int k, ok = 1;
        char kb[64];
        for (k = 0; k < 2000; k++) {
            if (ext2_read_file(3, 0, kb, sizeof(kb)) <= 0) {
                ok = 0;
                break;
            }
        }
        {
            double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
            double ops = sec > 0 ? 2000.0 / sec : 0;
            printf("40J kiyas: 2000 okuma %.3fsn (%.0f op/sn)\n", sec, ops);
            CHECK(ok && ops > 0, "40J benchmark");
        }
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
