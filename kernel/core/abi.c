/* 30.3: ABI dondurma denetimi.
 * Kritik syscall numaraları tarihsel adlarıyla mühürlüdür; yeniden
 * numaralandırma bu denetimi kırar (bilinçli karar gerektirir).
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "core/abi.h"
#include "core/doc.h"
#include "core/verify.h"

typedef struct {
    uint32_t nr;
    const char* name;
} abi_frozen_t;

static const abi_frozen_t frozen[] = {
    {1, "WRITE"},
    {12, "FORK"},
    {15, "EXEC"},
    {131, "BLKREAD"},
    {141, "SETUID"},
    {161, "CAPGET"},
    {163, "SB_ALLOW"},
    {166, "DOCNAME"},
};

#define FROZEN_N (sizeof(frozen) / sizeof(frozen[0]))

/* 30.3: derleme-zamanı kanıtı (mühür boş değil) */
STATIC_ASSERT((sizeof(frozen) / sizeof(frozen[0])) > 0);

static int streq(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

int abi_frozen_check(void) {
    for (uint32_t i = 0; i < FROZEN_N; i++) {
        const char* got = doc_name(frozen[i].nr);
        if (REQUIRE(got != 0, 0xA101) != 0) return -1;
        if (REQUIRE(streq(got, frozen[i].name), 0xA102) != 0) return -2;
    }
    return 0;
}

int abi_selftest(void) {
    if (abi_frozen_check() != 0) return -1;
    if (doc_selftest() != 0) return -2;
    return 0;
}
