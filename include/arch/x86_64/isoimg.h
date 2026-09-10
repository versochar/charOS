#ifndef ISOIMG64_H
#define ISOIMG64_H
#include "arch/x86_64/longmode.h"

#define ISOIMG64_MAX_FILES 512
#define ISOIMG64_NAME_MAX   48
#define ISOIMG64_SECT       2048
#define ISOIMG64_MAGIC      0x49534F36UL /* "ISO6" */

enum isoimg64_boot {
    ISOIMG_NONE = 0,
    ISOIMG_ELTORITO_FLOPPY = 1,
    ISOIMG_ELTORITO_NOT_EMULATE = 2,
};

int isoimg64_init(void);
int isoimg64_add_file(const char *name, u64 size_bytes);
int isoimg64_set_boot(const char *kernel_path, int boot_mode);
int isoimg64_layout(u64 *total_bytes);
int isoimg64_sector_for(const char *name, u32 *sector);
int isoimg64_checksum(void);
int isoimg64_verify(const char *name);
int isoimg64_file_count(void);
int isoimg64_boot_mode(int *out);

#endif