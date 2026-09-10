/* 28.3: Öztest kayıt defteri mantığı.
 * Dış bağımlılık yok (ad karşılaştırma yerleşik); çekirdek + host derlenir.
 */
#include "test/selftest.h"
#include "core/verify.h"

/* 28.3: derleme-zamanı kanıtı */
STATIC_ASSERT(SELFTEST_MAX > 0 && SELFTEST_MAX <= 64);

typedef struct {
    const char* name;
    selftest_fn fn;
} selftest_entry_t;

static selftest_entry_t entries[SELFTEST_MAX];
static uint32_t nentries = 0;

static int name_eq(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

void selftest_init(void) {
    nentries = 0;
    for (int i = 0; i < SELFTEST_MAX; i++) {
        entries[i].name = 0;
        entries[i].fn = 0;
    }
}

int selftest_register(const char* name, selftest_fn fn) {
    if (REQUIRE(name != 0 && name[0] != '\0', 0xE101) != 0) return -1;
    if (REQUIRE(fn != 0, 0xE102) != 0) return -1;
    if (REQUIRE(nentries < SELFTEST_MAX, 0xE103) != 0) return -1;
    for (uint32_t i = 0; i < nentries; i++) {
        if (REQUIRE(!name_eq(entries[i].name, name), 0xE104) != 0) return -1;
    }
    entries[nentries].name = name;
    entries[nentries].fn = fn;
    nentries++;
    return 0;
}

int selftest_run_all(void) {
    int fails = 0;
    for (uint32_t i = 0; i < nentries; i++) {
        if (entries[i].fn() != 0) fails++;
    }
    return fails;
}

uint32_t selftest_count(void) {
    return nentries;
}
