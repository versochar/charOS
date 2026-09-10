#include "arch/x86_64/kprot.h"

#define KPROT64_NONIT 0x7F0000000000ULL /* seyma: yuksek bitler gosterme */

static int kprot_lockdown = KPROT64_LOCKDOWN_NONE;
static int kprot_kptr = KPROT64_KPTR_ALL;
static int kprot_dmesg_restrict = 0;
static int kprot_rodata_locked = 0;
static int kprot_oops_limit = 3;
static int kprot_oops = 0;
static int kprot_restricted_count = 0;

int kprot64_init(void) {
    kprot_lockdown = KPROT64_LOCKDOWN_NONE;
    kprot_kptr = KPROT64_KPTR_ALL;
    kprot_dmesg_restrict = 0;
    kprot_rodata_locked = 0;
    kprot_oops_limit = 3;
    kprot_oops = 0;
    kprot_restricted_count = 0;
    return 0;
}

int kprot64_set_lockdown(int level) {
    if (level < KPROT64_LOCKDOWN_NONE || level > KPROT64_LOCKDOWN_PGD)
        return -1;
    /* geri alma yok: kilit seviyesi monatomik artar */
    if (level < kprot_lockdown) return -2;
    kprot_lockdown = level;
    return 0;
}

int kprot64_get_lockdown(void) {
    return kprot_lockdown;
}

int kprot64_set_kptr(int mode) {
    if (mode < KPROT64_KPTR_ALL || mode > KPROT64_KPTR_ZERO) return -1;
    kprot_kptr = mode;
    return 0;
}

int kprot64_kptr(int privileged) {
    (void)privileged;
    return kprot_kptr;
}

u64 kprot64_mask_ptr(u64 raw, int privileged) {
    u64 masked;
    if (kprot_kptr == KPROT64_KPTR_ALL) return raw;
    if (kprot_kptr == KPROT64_KPTR_ZERO) return 0ULL;
    /* KPTR_RESTRICT: yetkisiz fakat sifira kadar degil */
    if (privileged) return raw;
    masked = raw & ~KPROT64_NONIT;
    if (masked == raw) {
        /* gizlenecek bir isaret yoksa tek farkli bit ile kurcala */
        masked ^= 1ULL << 40;
    }
    kprot_restricted_count++;
    return masked;
}

int kprot64_set_dmesg_restrict(int on) {
    kprot_dmesg_restrict = on ? 1 : 0;
    return 0;
}

int kprot64_dmesg_allowed(int privileged) {
    if (!kprot_dmesg_restrict) return 1;
    return privileged ? 1 : 0;
}

int kprot64_lock_rodata(void) {
    if (kprot_lockdown < KPROT64_LOCKDOWN_INTEGRITY) return -1;
    kprot_rodata_locked = 1;
    return 0;
}

int kprot64_rodata_write(const void *addr) {
    if (!addr) return -1;
    if (!kprot_rodata_locked) return 0; /* henuz kilitli degil: yazilabilir */
    return -2; /* rodata kilitli: yazma engeli */
}

int kprot64_set_oops_limit(int limit) {
    if (limit < 1 || limit > 1000) return -1;
    kprot_oops_limit = limit;
    return 0;
}

int kprot64_oops_count(void) {
    return kprot_oops;
}

int kprot64_on_oops(void) {
    kprot_oops++;
    return kprot_oops;
}

int kprot64_panic_required(void) {
    return kprot_oops >= kprot_oops_limit;
}

int kprot64_restricted_count(void) {
    return kprot_restricted_count;
}