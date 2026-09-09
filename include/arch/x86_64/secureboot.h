#ifndef SECUREBOOT_H
#define SECUREBOOT_H

#include "arch/x86_64/longmode.h"

typedef struct {
    u8 hash[32];
    u64 pcr_index;
    u64 timestamp;
    u8 event_type;
} secureboot_event_t;

typedef enum {
    SB_OK = 0,
    SB_ERR_PARAM = -1,
    SB_ERR_INVALID_SIG = -2,
    SB_ERR_CRYPTO = -3,
} secureboot_err_t;

int secureboot64_init(void);
int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len);
int secureboot64_measure_kernel(const void *kernel, u64 size);
void secureboot64_log_event(const char *event);
int secureboot64_get_last_event(secureboot_event_t *out);

#endif
