/* 23.3: Gerçek sandbox bitmask mantığı.
 * task yapısına bağımlı DEĞİL: aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "process/sandbox.h"

void sb_allow_all(uint32_t* mask) {
    if (!mask) return;
    for (int i = 0; i < SB_MASK_WORDS; i++) mask[i] = 0xFFFFFFFFu;
}

void sb_lockdown(uint32_t* mask) {
    if (!mask) return;
    for (int i = 0; i < SB_MASK_WORDS; i++) mask[i] = 0;
}

int sb_allow(uint32_t* mask, uint32_t nr) {
    if (!mask || nr >= SB_MAX_NR) return -1;
    mask[nr / 32] |= (1u << (nr % 32));
    return 0;
}

int sb_deny(uint32_t* mask, uint32_t nr) {
    if (!mask || nr >= SB_MAX_NR) return -1;
    mask[nr / 32] &= ~(1u << (nr % 32));
    return 0;
}

int sb_check(const uint32_t* mask, uint32_t nr) {
    if (!mask || nr >= SB_MAX_NR) return 0;
    return (mask[nr / 32] & (1u << (nr % 32))) != 0;
}
