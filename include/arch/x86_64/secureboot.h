#ifndef SECUREBOOT_H
#define SECUREBOOT_H

#include "arch/x86_64/longmode.h"

int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len);
int secureboot64_measure_kernel(const void *kernel, u64 size);
void secureboot64_log_event(const char *event);

#endif
