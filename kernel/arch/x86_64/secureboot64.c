#include "arch/x86_64/longmode.h"

static u8 secureboot_key[64];
static int secureboot_initialized = 0;
static secureboot_event_t last_event;
static u8 pcr[32] = {0};

static void secureboot_init_key(void) {
    for (int i = 0; i < 64; i++) {
        secureboot_key[i] = (u8)i;
    }
    secureboot_initialized = 1;
}

/* Sabit zamanlı bellek karşılaştırma */
static int consttime_memcmp(const u8 *a, const u8 *b, u64 n) {
    u8 diff = 0;
    for (u64 i = 0; i < n; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff;
}

int secureboot64_init(void) {
    secureboot_init_key();
    for (int i = 0; i < 32; i++) {
        last_event.hash[i] = 0;
        pcr[i] = 0;
    }
    last_event.pcr_index = 1;
    last_event.timestamp = 0;
    last_event.event_type = 0;
    return SB_OK;
}

int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len) {
    if (!data || !sig || len == 0 || sig_len == 0) {
        return SB_ERR_PARAM;
    }
    if (!secureboot_initialized) {
        secureboot_init_key();
    }
    /* RSA-4096 imzası 512 byte olmalı */
    if (sig_len != 512) {
        return SB_ERR_INVALID_SIG;
    }
    /* Basit XOR hash simülasyonu - gerçekte RSA-PSS */
    u8 expected = 0;
    for (u64 i = 0; i < len; i++) {
        expected ^= data[i];
    }
    if (consttime_memcmp(&sig[0], &expected, 1) != 0) {
        return SB_ERR_INVALID_SIG;
    }
    secureboot64_log_event("verify_ok");
    return SB_OK;
}

int secureboot64_measure_kernel(const void *kernel, u64 size) {
    if (!kernel || size == 0) {
        return SB_ERR_PARAM;
    }
    u8 hash[32] = {0};
    const u8 *p = (const u8 *)kernel;
    for (u64 i = 0; i < size; i++) {
        hash[i % 32] ^= p[i];
    }
    /* PCR genişletme: PCR = SHA256(PCR || hash) - simülasyon */
    for (int i = 0; i < 32; i++) {
        pcr[i] ^= hash[i];
    }
    for (int i = 0; i < 32; i++) last_event.hash[i] = pcr[i];
    last_event.pcr_index = 1;
    last_event.timestamp = 1;
    last_event.event_type = 2;
    return SB_OK;
}

void secureboot64_log_event(const char *event) {
    (void)event;
    last_event.event_type = 1;
}

int secureboot64_get_last_event(secureboot_event_t *out) {
    if (!out) return SB_ERR_PARAM;
    *out = last_event;
    return SB_OK;
}

int secureboot64_get_pcr(u8 *out, u64 len) {
    if (!out || len < 32) return SB_ERR_PARAM;
    for (int i = 0; i < 32; i++) out[i] = pcr[i];
    return SB_OK;
}

