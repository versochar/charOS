#ifndef CHAROS_DRIVERS_NVME_H
#define CHAROS_DRIVERS_NVME_H

#include <stdint.h>

/* 27A: NVMe 1.3 host sürücüsü (polling, tek I/O kuyruk çifti).
 * Gerçek hedef: Micron 2210 (PCI 02:00.0); QEMU'da `-device nvme` ile test.
 * DMA: identity-map'li düşük bellek (statik tamponlar, virt==phys). */

/* Kurulum: 0 hazır, -1 cihaz yok/hata */
int nvme_init(void);
int nvme_present(void);

/* Toplam 512B-sektör eşdeğeri kapasite (0 = yok) */
uint64_t nvme_capacity_sectors(void);

/* 512B-sektör R/W (yerel LBA boyutu ne olursa olsun; iç kopya var).
 * 0 başarı, -1 hata. */
int nvme_read_sector(uint32_t lba512, void* buf);
int nvme_write_sector(uint32_t lba512, const void* buf);

/* Çoklu sektör (say512 adet 512B sektör). 0 başarı, -1 hata. */
int nvme_read_multi(uint32_t lba512, void* buf, uint32_t say512);
int nvme_write_multi(uint32_t lba512, const void* buf, uint32_t say512);

/* Boot self-test: scratch LBA RW karşılaştırma. 0 PASS, -1 FAIL. */
int nvme_selftest(void);

#endif
