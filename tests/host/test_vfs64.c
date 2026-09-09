/* 39J: vfsops64/dentry64/mountns64/symhard64/flock64/inotify64/poll64/
 *      ioctl64/tmpfs64 host testi.
 * Calistirma: make test-vfs64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

#define MAXMAP 1024
static u64 map_frame[MAXMAP];
static void *map_ptr[MAXMAP];
static int map_n = 0;

static void *test_mapper(u64 frame) {
    int i;
    void *p;
    for (i = 0; i < map_n; i++)
        if (map_frame[i] == frame) return map_ptr[i];
    p = malloc(4096);
    memset(p, 0, 4096);
    if (map_n < MAXMAP) {
        map_frame[map_n] = frame;
        map_ptr[map_n] = p;
        map_n++;
    }
    return p;
}

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static int ioctl_flag = 0;
static int ioctl_echo(u64 arg) {
    ioctl_flag = (int)arg;
    return (int)(arg * 2);
}

int main(void) {
    int i;
    u64 ino = 0, ino2 = 0, got = 0;
    int type = 0;
    u32 mode = 0;
    u64 size = 0;
    char ibuf[80], obuf[256];

    pmm64_set_frame_mapper(test_mapper);
    pmm64_init();
    pmm64_add_region(0, 64ULL * 1024 * 1024);

    CHECK(vfsops64_create(VFS64_FILE, 0644, &ino) == 0 && ino, "39A olustur");
    CHECK(vfsops64_lookup(ino, &type, &mode, &size) == 0 && type == 1 &&
          mode == 0644, "39A bak");
    CHECK(vfsops64_setsize(ino, 100) == 0, "39A boyut");
    CHECK(vfsops64_lookup(ino, 0, 0, &size) == 0 && size == 100,
          "39A boyut oku");
    CHECK(vfsops64_lookup(99999, 0, 0, 0) != 0, "39A yok red");
    CHECK(vfsops64_create(99, 0, 0) != 0, "39A tur red");

    (void)ino2;
    {
        u64 f;
        CHECK(vfsops64_create(VFS64_FILE, 0644, &f) == 0, "39B inode");
        CHECK(dentry64_insert(1, "not", f) == 0, "39B ekle");
        CHECK(dentry64_lookup(1, "not", &got) == 0 && got == f,
              "39B bul");
        CHECK(dentry64_lookup(1, "yok", 0) != 0, "39B miss");
        CHECK(dentry64_lookup(2, "not", 0) != 0, "39B ebeveyn ayirir");
        CHECK(dentry64_invalidate(1, "not") == 0, "39B gecersizle");
        CHECK(dentry64_lookup(1, "not", 0) != 0, "39B silindi");
        CHECK(dentry64_count() == 0, "39B sayac");
    }

    CHECK(mountns64_mount("/mnt/usb", "vfat", 7) == 0, "39C bagla");
    CHECK(mountns64_mount("/mnt/usb/disk", "ext2", 9) == 0, "39C ic ice");
    CHECK(mountns64_resolve("/mnt/usb/disk/a.txt", ibuf, sizeof(ibuf)) >=
              0 && !strcmp(ibuf, "/a.txt"), "39C en uzun");
    CHECK(mountns64_resolve("/mnt/usb/b.txt", ibuf, sizeof(ibuf)) >= 0 &&
          !strcmp(ibuf, "/b.txt"), "39C dis");
    CHECK(mountns64_unmount("/mnt/usb/disk") == 0, "39C coz");
    CHECK(mountns64_resolve("/mnt/usb/disk/a.txt", ibuf, sizeof(ibuf)) >=
              0 && !strcmp(ibuf, "/disk/a.txt"), "39C coz sonrasi");
    CHECK(mountns64_resolve("goreceli", 0, 0) != 0, "39C goreceli red");

    {
        u64 sl = 0;
        CHECK(symhard64_symlink("/bin/sh", &sl) == 0 && sl, "39D symlink");
        CHECK(symhard64_readlink(sl, obuf, sizeof(obuf)) == 0 &&
              !strcmp(obuf, "/bin/sh"), "39D readlink");
        CHECK(symhard64_readlink(99999, obuf, sizeof(obuf)) != 0,
              "39D yok red");
        CHECK(symhard64_hardlink(ino) == 2, "39D hardlink nlink=2");
        CHECK(vfsops64_nlink(ino) == 2, "39D nlink oku");
        CHECK(vfsops64_unlink(ino) == 1, "39D unlink kalan=1");
    }

    CHECK(flock64_lock(10, 1, FLOCK64_EX, 0, 100) == 0, "39E ozel kilit");
    CHECK(flock64_check(10, 2, FLOCK64_SH, 50, 10) != 0, "39E cakisma");
    CHECK(flock64_check(10, 1, FLOCK64_SH, 50, 10) == 0,
          "39E sahip muaf");
    CHECK(flock64_check(10, 2, FLOCK64_SH, 200, 10) == 0,
          "39E ayrik serbest");
    CHECK(flock64_lock(10, 2, FLOCK64_SH, 0, 100) != 0, "39E kilit red");
    CHECK(flock64_unlock(10, 1) == 0, "39E birak");
    CHECK(flock64_lock(10, 2, FLOCK64_SH, 0, 100) == 0, "39E sonra serbest");
    CHECK(flock64_lock(10, 3, FLOCK64_SH, 0, 100) == 0,
          "39E paylasimli ortak");
    CHECK(flock64_unlock(10, 2) == 0 && flock64_unlock(10, 3) == 0,
          "39E toplu birak");

    {
        int wd = inotify64_add_watch(42, 0xFF);
        u64 ei;
        u32 em;
        CHECK(wd >= 0, "39F izle");
        CHECK(inotify64_add_watch(42, 0x0F) == wd, "39F maske birlesir");
        CHECK(inotify64_event(42, 0x01) == 1, "39F olay");
        CHECK(inotify64_event(99, 0x01) == 0, "39F izlenmeyen sessiz");
        CHECK(inotify64_read(wd, &ei, &em) == 0 && ei == 42 && em == 0x01,
              "39F oku");
        CHECK(inotify64_read(wd, 0, 0) != 0, "39F bos red");
    }

    {
        int fds[2] = {3, 5};
        u32 rev[2] = {0, 0};
        CHECK(poll64_set(3, 0x01) == 0, "39G hazirla");
        CHECK(poll64_poll(fds, 2, 0x01, rev) == 1 && rev[0] == 0x01 &&
              rev[1] == 0, "39G poll");
        {
            int ep = epoll64_create();
            int of[4];
            u32 oe[4];
            CHECK(ep >= 0, "39G epoll");
            CHECK(epoll64_add(ep, 3, 0x01) == 0, "39G ekle");
            CHECK(epoll64_add(ep, 3, 0x01) != 0, "39G cift red");
            CHECK(epoll64_mod(ep, 3, 0x03) == 0, "39G degistir");
            CHECK(epoll64_mod(ep, 9, 0x01) != 0, "39G yok red");
            CHECK(epoll64_wait(ep, of, oe, 4) == 1 && of[0] == 3,
                  "39G bekle");
            CHECK(epoll64_del(ep, 3) == 0, "39G sil");
            CHECK(epoll64_wait(ep, of, oe, 4) == 0, "39G bos");
        }
    }

    CHECK(ioctl64_register(8, 0x100, ioctl_echo) == 0, "39H kayit");
    ioctl_flag = 0;
    CHECK(ioctl64_call(8, 0x100, 21) == 42 && ioctl_flag == 21,
          "39H cagri");
    CHECK(ioctl64_call(8, 0x101, 0) != 0, "39H komut yok");
    CHECK(ioctl64_call(9, 0x100, 0) != 0, "39H cihaz yok");

    CHECK(tmpfs64_create("not.txt") == 0, "39I olustur");
    CHECK(tmpfs64_create("not.txt") != 0, "39I cift red");
    {
        const char *txt = "charOS tmpfs merhaba dunya, 5KB'yi asan uzun icerik "
                          "-blogu-0123456789";
        char back[256];
        memset(back, 0, sizeof(back));
        CHECK(tmpfs64_write("not.txt", 0, txt, strlen(txt)) == 0,
              "39I yaz");
        CHECK(tmpfs64_size("not.txt") == (u64)strlen(txt), "39I boyut");
        CHECK(tmpfs64_read("not.txt", 0, back, sizeof(back)) ==
                  (int)strlen(txt) && !strcmp(back, txt), "39I oku");
        CHECK(tmpfs64_write("not.txt", 5000, "X", 1) == 0,
              "39I delikli yazma");
        CHECK(tmpfs64_size("not.txt") == 5001, "39I delikli boyut");
        CHECK(tmpfs64_read("not.txt", 5000, back, 1) == 1 && back[0] == 'X',
              "39I delikli oku");
        CHECK(tmpfs64_unlink("not.txt") == 0, "39I sil");
        CHECK(tmpfs64_size("not.txt") == 0, "39I sil sonrasi");
    }

    for (i = 0; i < map_n; i++) free(map_ptr[i]);
    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
