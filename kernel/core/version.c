/* 27.3: Sürüm dizgileri salt-okunur veride yaşar.
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "core/version.h"

#ifndef CHAROS_VERSION_STR
#define CHAROS_VERSION_STR "charOS dev"
#endif
#ifndef CHAROS_VERSION_COMMIT
#define CHAROS_VERSION_COMMIT "nogit"
#endif

static const char vstr[] = CHAROS_VERSION_STR;
static const char vcom[] = CHAROS_VERSION_COMMIT;

const char* version_string(void) {
    return vstr;
}

const char* version_commit(void) {
    return vcom;
}
