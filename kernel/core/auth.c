/* 37.3: Kimlik doğrulama mantığı (gerçek çekirdek kodu).
 * Token türetimi sabit + ad uzunluğuna bağlı; doğrulama karşılaştır.
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "core/auth.h"
#include "core/verify.h"

/* 37.3: derleme-zamanı kanıtı (boş değil) */
STATIC_ASSERT(sizeof(uint32_t) == 4);

static int auth_active = 0;

void auth_init(void) {
    auth_active = 1;
}

uint32_t auth_token_gen(const char* name, uint32_t seed) {
    uint32_t len = 0;
    uint32_t tok = seed ^ 0x8C7A;
    if (REQUIRE(name != 0, 0xE901) != 0) return 0; /* 37.4: sözleşme */
    while (name[len] != '\0') len++;
    tok ^= len;
    tok = (tok << 3) | (tok >> 29);
    return tok;
}

int auth_check(const char* name, uint32_t token) {
    if (REQUIRE(auth_active, 0xE902) != 0) return AUTH_FAIL; /* 37.4 */
    if (REQUIRE(name != 0, 0xE903) != 0) return AUTH_FAIL; /* 37.4 */
    uint32_t exp = auth_token_gen(name, 0xDEADBEEF);
    int match = (token == exp ? 1 : 0);
    return match ? AUTH_OK : AUTH_FAIL;
}
