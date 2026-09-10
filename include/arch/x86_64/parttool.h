#ifndef PARTTOOL64_H
#define PARTTOOL64_H
#include "arch/x86_64/longmode.h"

#define PARTTOOL64_MAX_DEVS   4
#define PARTTOOL64_MAX_PARTS  16
#define PARTTOOL64_NAME_MAX   40

/* GPT tip kodu ornekleri */
#define PARTTOOL_TYPE_EFI    0xEF // ESP
#define PARTTOOL_TYPE_MICRO  0x0C // FAT32 LBA
#define PARTTOOL_TYPE_EXT4   0x83 // Linux
#define PARTTOOL_TYPE_SWAP   0x82

int parttool64_init(void);
int parttool64_gpt_init(u64 dev_id, u64 total_lba);
int parttool64_gpt_add(u64 dev_id, const char *name, int type_code,
                       u64 first_lba, u64 last_lba);
int parttool64_gpt_info(u64 dev_id, int index, char *name_out, int name_max,
                        int *type_out, u64 *first_out, u64 *last_out);
int parttool64_gpt_delete(u64 dev_id, int index);
int parttool64_gpt_crc(u64 dev_id);
int parttool64_gpt_repair(u64 dev_id);
int parttool64_gpt_count(u64 dev_id);

#endif