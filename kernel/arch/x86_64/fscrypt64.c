#include "arch/x86_64/longmode.h"
#include "arch/x86_64/fscrypt.h"

#define FSCRYPT_GOLDEN 0x9E3779B97F4A7C15ULL

static int fs_has_key = 0;
static u64 fs_key = 0;

int fscrypt64_init(void) {
    fs_has_key = 0;
    fs_key = 0;
    return 0;
}

int fscrypt64_setkey(u64 key) {
    fs_key = key;
    fs_has_key = 1;
    return 0;
}

int fscrypt64_encrypt(u64 plain, u64 *out_cipher) {
    if (!fs_has_key) return -2;
    if (!out_cipher) return -1;
    *out_cipher = plain ^ fs_key ^ FSCRYPT_GOLDEN;
    return 0;
}

int fscrypt64_decrypt(u64 cipher, u64 *out_plain) {
    if (!fs_has_key) return -2;
    if (!out_plain) return -1;
    *out_plain = cipher ^ fs_key ^ FSCRYPT_GOLDEN;
    return 0;
}

int fscrypt64_wipe(void) {
    fs_key = 0;
    fs_has_key = 0;
    return 0;
}
