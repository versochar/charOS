#ifndef TPM_H
#define TPM_H

#include "arch/x86_64/longmode.h"

int tpm64_init(void);
int tpm64_extend_pcr(u64 pcr_idx, const u8 *hash, u64 hash_len);
int tpm64_read_pcr(u64 pcr_idx, u8 *out);
int tpm64_quote(u8 *quote, u64 *len);

#endif
