#ifndef CHAROS_CORE_VERIFY_H
#define CHAROS_CORE_VERIFY_H

/* 24.2: Sözleşme (contract) katmanı.
 * - Derleme zamanı: STATIC_ASSERT (gnu99 uyumlu, _Static_assert yok).
 * - Çalışma zamanı: verify_require (kurtarılabilir: -1 + hook),
 *   hook mekanizması çekirdekte syslog'a, host'ta test kancasına bağlanır.
 * Bağımlılık: yalnızca stdint (freestanding-safe, host'ta da derlenir).
 */
#include "stdint.h"

/* İki seviyeli yapıştırma (satır numarası benzersizliği için gerekli) */
#define VERIFY_JOIN_(a, b) a##b
#define VERIFY_JOIN(a, b) VERIFY_JOIN_(a, b)
#define STATIC_ASSERT(cond) \
    extern char VERIFY_JOIN(static_assert_at_line_, __LINE__)[(cond) ? 1 : -1]

typedef void (*verify_hook_fn)(uint32_t code, const char* file, uint32_t line);

void     verify_set_hook(verify_hook_fn fn);
int      verify_require(int cond, uint32_t code, const char* file, uint32_t line);
uint32_t verify_fail_count(void);
int      verify_selftest(void); /* 0=ok; açılışta ve host'ta çalışır */

#define REQUIRE(cond, code) verify_require((cond), (code), __FILE__, __LINE__)

#endif
