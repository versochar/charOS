#include "arch/x86_64/longmode.h"
#include "arch/x86_64/blk.h"

#define BLK_MAX 8
#define BLK_SECTORS 128

typedef struct {
    u64 dev;
    u64 nsectors;
    u64 data[BLK_SECTORS];
    int active;
} blk_dev_t;

static int blk_inited = 0;
static u64 blk_ctr = 0;
static blk_dev_t disks[BLK_MAX];

static blk_dev_t *find_blk(u64 dev) {
    for (int i = 0; i < BLK_MAX; i++) {
        if (disks[i].active && disks[i].dev == dev) return &disks[i];
    }
    return 0;
}

int blk64_init(void) {
    blk_inited = 1;
    blk_ctr = 0;
    for (int i = 0; i < BLK_MAX; i++) {
        disks[i].dev = 0; disks[i].nsectors = 0; disks[i].active = 0;
        for (int j = 0; j < BLK_SECTORS; j++) disks[i].data[j] = 0;
    }
    return 0;
}

int blk64_create(u64 nsectors, u64 *out_dev) {
    if (!blk_inited || !out_dev || nsectors == 0 || nsectors > BLK_SECTORS) return -1;
    for (int i = 0; i < BLK_MAX; i++) {
        if (!disks[i].active) {
            blk_ctr++;
            disks[i].dev = blk_ctr;
            disks[i].nsectors = nsectors;
            disks[i].active = 1;
            for (u64 j = 0; j < nsectors; j++) disks[i].data[j] = 0;
            *out_dev = blk_ctr;
            return 0;
        }
    }
    return -1;
}

int blk64_destroy(u64 dev) {
    blk_dev_t *d = find_blk(dev);
    if (!d) return -1;
    d->active = 0; d->dev = 0;
    return 0;
}

int blk64_read(u64 dev, u64 lba, u64 *out_val) {
    blk_dev_t *d = find_blk(dev);
    if (!d || !out_val || lba >= d->nsectors) return -1;
    *out_val = d->data[lba];
    return 0;
}

int blk64_write(u64 dev, u64 lba, u64 val) {
    blk_dev_t *d = find_blk(dev);
    if (!d || lba >= d->nsectors) return -1;
    d->data[lba] = val;
    return 0;
}

int blk64_size(u64 dev, u64 *out_nsectors) {
    blk_dev_t *d = find_blk(dev);
    if (!d || !out_nsectors) return -1;
    *out_nsectors = d->nsectors;
    return 0;
}
