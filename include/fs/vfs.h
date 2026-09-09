#ifndef CHAROS_FS_VFS_H
#define CHAROS_FS_VFS_H

#include <stdint.h>
#include <core/syscall.h>
#include <fs/fd.h>

/* chFS - Char File System (basit ramfs/VFS) */

#define FS_MAX_PATH 256
#define FS_MAX_FILES 64
#define FS_MAX_BLOCKS 256
#define FS_BLOCK_SIZE 512

/* Dosya türleri */
#define FT_FILE 1
#define FT_DIR  2

/* Açık mod */
#define O_RDONLY  1
#define O_WRONLY  2
#define O_RDWR    3
#define O_CREATE  4
#define O_TRUNC   8   /* 19E: açarken boyu sıfırla (blokları serbest bırakır) */
#define O_APPEND  16  /* 19E: ofseti dosya sonuna kur */

/* İzin bitleri */
#define PERM_R  4  // read (100)
#define PERM_W  2  // write (010)
#define PERM_X  1  // execute (001)

/* Dizin girişi (directory entry) */
struct dirent {
    char name[24];
    int inode_id;
};

/* Inode (dosya meta verisi) */
#define CHFS_DATABLOCKS 64      /* 19F: dosya başına sektör (32KB max) */
struct inode {
    int id;                 // inode numarası
    int type;               // FT_FILE veya FT_DIR
    int size;               // byte
    int blocks[CHFS_DATABLOCKS]; // blok indeksleri (max 24*512 = 12KB)
    int permissions;        // PERM_R/W/X (izin bitleri, tarihsel)
    int uid;                // 14E: sahip kullanıcı
    int gid;                // 14E: sahip grup
    int mode;               // 14E: unix tarzı mod (örn 0644)
    char name[32];          // dosya adı
    int ref_count;          // açık fd sayısı
};

/* Süper blok (dosya sistemi genel bilgisi) */
struct superblock {
    int magic;              // "CHFS" magic
    int total_blocks;
    int free_blocks;
    int inode_count;
};

/* fd_entry ve FD_* artık fs/fd.h'de */

/* VFS arayüzü */
void fs_init(void);
int fs_open(const char* path, int mode);
int fs_read(int fd, char* buf, int len);
int fs_write(int fd, const char* buf, int len);
int fs_close(int fd);
int fs_dup2(int oldfd, int newfd); /* 14C */
int fs_can(int uid, int gid, struct inode* ino, int req); /* 14E: req PERM_* */
int fs_can_path(const char* path, int req); /* 14E: o anki task ile */
int fs_chmod(const char* path, int mode);   /* 14E: root/sahip */
int fs_create(const char* path);
int fs_mkdir(const char* path);
void fs_list(void);

int fs_unlink(const char* path);

/* Syscall entegrasyonu için */
int vfs_sys_open(const char* path, int mode);

#endif
