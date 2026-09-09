#ifndef CHAROS_FS_DISKFS_H
#define CHAROS_FS_DISKFS_H

#include <stdint.h>

/* 13H: virtio-blk üstünde kalıcı mini FS (superblock/inode/bitmap).
 * 512B sektörler. v2 (17 serisi): dizinler, büyük dosyalar, silme,
 * truncate, bütünlük denetimi, istatistik.
 *  17A: dizin ağacı (prefix path), 17B: çok bloklu büyük dosya,
 *  17C: df_remove + blok/kapasite geri kazanımı, 17D: truncate/update,
 *  17E: reboot arası kalıcılık, 17F: liste/stat API'leri,
 *  17G: inode yeniden kullanım stresi, 17H: bütünlük + hata yolları. */

#define DISKFS_MAGIC   0x31484443u   /* "CHD1" */
#define DISKFS_VERSION 2
#define DISKFS_INODES   64
#define DISKFS_DATABLOCKS 48          /* inode başına direkt sektör (24KB) */
#define DISKFS_MAXFILE  (DISKFS_DATABLOCKS * 512)

#define DFS_TYPE_FILE  0
#define DFS_TYPE_DIR   1

struct dfs_stat {
    uint32_t size;      /* bayt */
    uint32_t is_dir;    /* 0/1 */
};

struct dfs_stats {
    uint32_t total_blocks;   /* disk sektörü */
    uint32_t used_blocks;
    uint32_t free_blocks;
    uint32_t used_inodes;
    uint32_t inode_count;
};

/* 13H (temel) */
int diskfs_mount(void);              /* 0 takılı, -1 sihir yok / hata */
int diskfs_format(void);             /* diski sıfırla + SB yaz */
/* 27D: blok soyutlama + bölüm içi mount/format */
void diskfs_use_nvme(int on);        /* 1=NVMe, 0=virtio-blk */
int diskfs_mount_at(uint32_t base, uint32_t max_sec);
int diskfs_format_at(uint32_t base, uint32_t total);
/* 27D: NVMe üstü GPT bölümünde kalıcılık testi */
int diskfs_nvme_selftest(void);
int dfile_write(const char* name, const char* data, int len);
int dfile_read(const char* name, char* buf, int maxlen);
void diskfs_list(void);
void diskfs_boot(void);              /* mount/format + selftest */

/* 17A */
int df_mkdir(const char* path);      /* dizin oluştur (0 ok, -1 hata) */
int df_readdir(const char* path, char* out, int max); /* '\n' ile ayrılmış alt öğeler */

/* 17C */
int df_remove(const char* path);     /* boş olmayan dizin reddedilir */

/* 17D */
int df_truncate(const char* path);   /* boyut 0 + blokları serbest bırak */

/* 17F */
int df_stat(const char* path, struct dfs_stat* out);
void df_stats(struct dfs_stats* out);

/* 27F: journal kurtarma + fsck (blk üzerinden) */
void diskfs_check_and_recover(void);
int diskfs_recover(void);              /* 0 tutarlı/kurtarıldı, -1 bozuk */

/* 17H */
int df_check(void);                  /* 0 tutarlı, -1 bozuk */

#endif