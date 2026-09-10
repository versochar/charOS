#ifndef CHAROS_TEST_SELFTEST_H
#define CHAROS_TEST_SELFTEST_H

/* 28.2: Açılış öztest kayıt defteri.
 * Dağınık `*_selftest()` çağrıları yerine: kaydet + toplu koş + özet.
 * Sözleşme: test fn 0=PASS, sıfır-dışı=FAIL. Aynı dosya iki yerde derlenir.
 */
#include "stdint.h"

#define SELFTEST_MAX 32

typedef int (*selftest_fn)(void);

void     selftest_init(void);
int      selftest_register(const char* name, selftest_fn fn); /* 0 ok / -1 */
int      selftest_run_all(void);   /* FAIL sayısı döner, kayıt sırasıyla koşar */
uint32_t selftest_count(void);

#endif
