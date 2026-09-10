#ifndef LUKSOP64_H
#define LUKSOP64_H
#include "arch/x86_64/longmode.h"

#define LUKSOP64_SLOTS 8
#define LUKSOP64_CIPHER_MAX 32
#define LUKSOP64_MAGIC 0x4C554B53UL /* "LUKS" */

enum luksop64_state {
    LUKSOP_CLOSED = 0,
    LUKSOP_UNLOCKED = 1,
    LUKSOP_CORRUPT = 2,
};

int luksop64_init(void);
int luksop64_format(u64 device, const char *cipher, int key_bits);
int luksop64_add_key_slot(int slot, u64 kdf_iter, u64 key_material);
int luksop64_verify_key(int slot, u64 key_material);
int luksop64_unlock(int slot, u64 key_material);
int luksop64_lock(void);
int luksop64_state(int *out);
int luksop64_header_crc(u64 device);

#endif