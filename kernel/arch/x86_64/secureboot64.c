#include "arch/x86_64/longmode.h"

static int secureboot_active = 0;

int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len) {
    (void)data; (void)len; (void)sig; (void)sig_len;
    return secureboot_active ? 1 : 0;
}

int secureboot64_measure_kernel(const void *kernel, u64 size) {
    (void)kernel; (void)size;
    return 0;
}

void secureboot64_log_event(const char *event) {
    (void)event;
}
