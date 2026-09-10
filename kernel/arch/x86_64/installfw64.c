#include "arch/x86_64/installfw.h"
#include <string.h>

#define INSTALLFW64_MIN_DISK (4ULL * 1024 * 1024 * 1024) /* 4 GiB */
#define INSTALLFW64_MIN_RAM  (512ULL * 1024 * 1024)      /* 512 MiB */

struct part_entry {
    char label[INSTALLFW64_NAME_MAX];
    u64  bytes;
    u8   created;
};
static struct part_entry parts[INSTALLFW64_MAX_STEPS];
static int part_n = 0;
static char disk_dev[INSTALLFW64_NAME_MAX];
static char boot_target[INSTALLFW64_NAME_MAX];
static u64 copy_total = 0;
static u64 copy_done = 0;
static int fw_step = INSTALLFW_PREFLIGHT;
static int fw_failed = 0;

int installfw64_init(void) {
    part_n = 0;
    copy_total = 0;
    copy_done = 0;
    fw_step = INSTALLFW_PREFLIGHT;
    fw_failed = 0;
    memset(parts, 0, sizeof(parts));
    memset(disk_dev, 0, sizeof(disk_dev));
    memset(boot_target, 0, sizeof(boot_target));
    return 0;
}

int installfw64_preflight(u64 disk_bytes, u64 ram_bytes) {
    if (fw_step != INSTALLFW_PREFLIGHT) return -1;
    if (disk_bytes < INSTALLFW64_MIN_DISK) { fw_failed = 1; return -2; }
    if (ram_bytes < INSTALLFW64_MIN_RAM) { fw_failed = 1; return -3; }
    fw_step = INSTALLFW_DISK;
    return 0;
}

int installfw64_select_disk(const char *dev, int wipe) {
    if (!dev) return -1;
    if (fw_step != INSTALLFW_DISK) return -2;
    if (wipe != 0 && wipe != 1) return -3;
    strncpy(disk_dev, dev, sizeof(disk_dev) - 1);
    fw_step = INSTALLFW_PARTITION;
    return 0;
}

int installfw64_create_partition(const char *label, u64 bytes) {
    if (!label || !bytes) return -1;
    if (fw_step != INSTALLFW_PARTITION) return -2;
    if (bytes < (32ULL * 1024 * 1024)) return -3; /* en az 32 MiB */
    if (part_n >= INSTALLFW64_MAX_STEPS) return -4;
    strncpy(parts[part_n].label, label, sizeof(parts[part_n].label) - 1);
    parts[part_n].bytes = bytes;
    parts[part_n].created = 1;
    part_n++;
    return 0;
}

int installfw64_copy_stage(const char *from, const char *to, u64 bytes) {
    if (!from || !to) return -1;
    if (fw_step == INSTALLFW_COPY) {
        copy_done += bytes;
        if (copy_done > copy_total) return -4;
        if (copy_done == copy_total) fw_step = INSTALLFW_BOOTLOADER;
        return 0;
    }
    if (fw_step != INSTALLFW_PARTITION) return -2;
    copy_total = bytes;
    copy_done = 0;
    fw_step = INSTALLFW_COPY;
    return 0;
}

int installfw64_install_bootloader(const char *target) {
    if (!target) return -1;
    if (fw_step != INSTALLFW_BOOTLOADER) return -2;
    strncpy(boot_target, target, sizeof(boot_target) - 1);
    fw_step = INSTALLFW_DONE;
    return 0;
}

int installfw64_finalize(void) {
    if (fw_step != INSTALLFW_DONE) return -1;
    return 0;
}

int installfw64_step(int *out) {
    if (!out) return -1;
    *out = fw_step;
    return 0;
}

int installfw64_partition_count(void) {
    return part_n;
}