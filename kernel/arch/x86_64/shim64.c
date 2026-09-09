/* 34F: Secure Boot shim politikasi — SetupMode/AuditMode matrisi.
 * Imza dogrulama 35x guvenlikte dolar; burada karar mantigi.
 */
#include "arch/x86_64/longmode.h"

int shim64_policy(int secure_boot, int setup_mode, int audit_mode) {
    if (!secure_boot) return 1;      /* SecureBoot kapali: serbest */
    if (setup_mode) return 1;        /* SetupMode: henuz kayit yok, serbest */
    if (audit_mode) return 2;        /* AuditMode: denetimli serbest */
    return 0;                        /* Zorunlu mod: imza gerekli (engelli) */
}
