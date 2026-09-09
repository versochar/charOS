#ifndef CHAROS_DRIVERS_BLK_H
#define CHAROS_DRIVERS_BLK_H

#include <stdint.h>

/* 27E: Blok soyutlama katmanı - virtio-blk + NVMe tek tip arayüz.
 * diskfs ve syscall bu API üzerinden çalışır; sürücü bağımsız FS. */

/* 0=hazır/ok, -1=yok/hata */
int blk_init(void);
int blk_present(void);

/* Sektör sayısı (512B sektör, 0=yok) */
uint32_t blk_capacity(void);

/* 512B sektör R/W (buf 512B, iç kopya veya güvenli adres gerektirir).
 * 0=ok, -1=hata */
int blk_read(uint32_t lba512, void* buf);
int blk_write(uint32_t lba512, const void* buf);

/* 27F: fsck çağrısı (diskfs df_check() ile aynı mantık).
 * 0=tutarlı, -1=bozuk (journal'dan kurtarma gerekebilir) */
int blk_fsck(void);

/* 27F: journal kurtarma (NVMe bölümünde veri bütünlüğü).
 * 0=kurtarıldı/tutarlı, -1=kurtarılamadı */
int blk_recover(void);

#endif
