#ifndef INSTALLFW64_H
#define INSTALLFW64_H
#include "arch/x86_64/longmode.h"

#define INSTALLFW64_MAX_STEPS 16
#define INSTALLFW64_NAME_MAX   48

enum installfw64_step {
    INSTALLFW_PREFLIGHT = 0,
    INSTALLFW_DISK      = 1,
    INSTALLFW_PARTITION = 2,
    INSTALLFW_COPY      = 3,
    INSTALLFW_BOOTLOADER= 4,
    INSTALLFW_DONE      = 5,
};

int installfw64_init(void);
int installfw64_preflight(u64 disk_bytes, u64 ram_bytes);
int installfw64_select_disk(const char *dev, int wipe);
int installfw64_create_partition(const char *label, u64 bytes);
int installfw64_copy_stage(const char *from, const char *to, u64 bytes);
int installfw64_install_bootloader(const char *target);
int installfw64_finalize(void);
int installfw64_step(int *out);
int installfw64_partition_count(void);

#endif