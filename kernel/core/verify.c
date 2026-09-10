/* 24.3: Sözleşme çalışma-zamanı.
 * Saf mantık + kanca sayacı; aynı dosya çekirdekte ve host testinde derlenir.
 * Çekirdek kancası kernel.c'de kurulur (syslog); varsayılan kanca yok.
 */
#include "core/verify.h"

static verify_hook_fn vhook = 0;
static uint32_t vfail = 0;

void verify_set_hook(verify_hook_fn fn) {
    vhook = fn;
}

int verify_require(int cond, uint32_t code, const char* file, uint32_t line) {
    if (cond) return 0;
    vfail++;
    if (vhook) vhook(code, file, line);
    return -1;
}

uint32_t verify_fail_count(void) {
    return vfail;
}

static void selftest_hook(uint32_t code, const char* file, uint32_t line) {
    (void)code; (void)file; (void)line;
}

int verify_selftest(void) {
    verify_hook_fn old = vhook;
    uint32_t base = vfail;
    vhook = selftest_hook;
    if (verify_require(1, 0xE001, __FILE__, __LINE__) != 0) { vhook = old; return -1; }
    if (verify_require(0, 0xE002, __FILE__, __LINE__) != -1) { vhook = old; return -2; }
    if (verify_fail_count() != base + 1) { vhook = old; return -3; }
    vhook = old;
    return 0;
}
