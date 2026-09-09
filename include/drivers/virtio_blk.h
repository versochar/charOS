#ifndef CHAROS_DRIVERS_VIRTIO_BLK_H
#define CHAROS_DRIVERS_VIRTIO_BLK_H

#include <stdint.h>

/* 13G: virtio-blk legacy sürücü (polling, tek kuyruk) + blok katmanı */

#define VIRTIO_SECTOR_SIZE 512

/* Kurulum: 0 hazır, -1 cihaz yok/hata */
int virtio_blk_init(void);
int virtio_blk_present(void);

/* Toplam sektör sayısı (0 = yok) */
uint32_t virtio_blk_capacity(void);

/* Tek sektör R/W (buf en az 512B, düşük bellekte olmalı değil - iç kopya var).
 * 0 başarı, -1 hata. */
int block_read_sector(uint32_t lba, void* buf);
int block_write_sector(uint32_t lba, const void* buf);

/* Boot self-test: scratch sektör RW karşılaştırma. 0 PASS, -1 FAIL. */
int virtio_blk_selftest(void);

#endif
