#include "arch/x86_64/longmode.h"
#include "arch/x86_64/tpm.h"

static u8 pcrs[24][32] = {0};
static int tpm_initialized = 0;

int tpm64_init(void) {
    tpm_initialized = 1;
    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 32; j++) {
            pcrs[i][j] = 0;
        }
    }
    return 0;
}

int tpm64_extend_pcr(u64 pcr_idx, const u8 *hash, u64 hash_len) {
    if (!tpm_initialized || pcr_idx >= 24 || !hash || hash_len != 32) return -1;
    for (int i = 0; i < 32; i++) {
        pcrs[pcr_idx][i] ^= hash[i];
    }
    return 0;
}

int tpm64_read_pcr(u64 pcr_idx, u8 *out) {
    if (!tpm_initialized || pcr_idx >= 24 || !out) return -1;
    for (int i = 0; i < 32; i++) {
        out[i] = pcrs[pcr_idx][i];
    }
    return 0;
}

int tpm64_quote(u8 *quote, u64 *len) {
    if (!quote || !len) return -1;
    *len = 0;
    return 0;
}
