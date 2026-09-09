#ifndef CHAROS_FS_CHFS_H
#define CHAROS_FS_CHFS_H

#include <fs/vfs.h>

/* 18K: dosya yöneticisi listeleme girişi */
#define FS_LIST_NAME_LEN 25
#define FS_LIST_MAX 64 /* flat chFS: tüm inode'lar (FS_MAX_FILES) listelenir */
struct fs_list_entry {
    int is_dir;
    char name[FS_LIST_NAME_LEN];
};

/* chFS syscall entegrasyonu */
void chfs_init(void);
int chfs_get_size(const char* path);
int chfs_read_file(const char* path, char* buf, int max_len);

/* 18K: GUI dosya yöneticisi için veri döndüren listeleme + taşıma */
int chfs_list_dir(const char* path, struct fs_list_entry* out, int max);
int chfs_move(const char* oldpath, const char* newpath);
/* 18S: editör için sıfırlama + fd'siz yazma */
int chfs_truncate(const char* path);
int chfs_write_file(const char* path, const char* buf, int len);

/* 19G: belirli task'in fd tablosunu kapat (sinyal-ölümü, task_current bağımsız) */
struct fd_entry;
void fs_close_table(struct fd_entry* tbl);

#endif
