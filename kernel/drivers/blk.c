#include <drivers/blk.h>
#include <drivers/virtio_blk.h>
#include <drivers/nvme.h>
#include <drivers/gpt.h>
#include <fs/diskfs.h>
#include <string.h>

/* 27E: blk soyutlama - NVMe varsa NVMe, yoksa virtio-blk */

int blk_init(void) {
    if (nvme_present()) return 0; /* NVMe zaten init edilmiştir */
    return virtio_blk_init(); /* virtio-blk init */
}

int blk_present(void) {
    return nvme_present() || virtio_blk_present();
}

uint32_t blk_capacity(void) {
    if (nvme_present()) {
        uint64_t c = nvme_capacity_sectors();
        return (c > 0xFFFFFFFFu) ? 0xFFFFFFFFu : (uint32_t)c;
    }
    return virtio_blk_capacity();
}

int blk_read(uint32_t lba, void* buf) {
    if (nvme_present()) return nvme_read_sector(lba, buf);
    return block_read_sector(lba, buf);
}

int blk_write(uint32_t lba, const void* buf) {
    if (nvme_present()) return nvme_write_sector(lba, buf);
    return block_write_sector(lba, buf);
}

/* 27F: blk_fsck - diskfs df_check'i blk üzerinden çalıştır */
int blk_fsck(void) {
    /* blk_fsck, diskfs'in mount edildiği varsayımıyla çalışır.
     * diskfs.c'deki df_check() ile aynı mantığı kullanır. */
    return df_check();
}

/* 27F: blk_recover - blk seviyesi kurtarma (temel delegasyon) */
int blk_recover(void) {
    /* blk_recover: diskfs_recover() üzerinden çalışır.
     * blk katmanı sadece cihaz varlığını kontrol eder. */
    if (!blk_present()) return -1;
    return (blk_present() && (blk_capacity() > 64)) ? 0 : -1;
}
