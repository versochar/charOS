/* 36.3: HDA komut kodlayıcı (saf bit alanları).
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "drivers/hdaverb.h"
#include "core/verify.h"

/* 36.3: derleme-zamanı kanıtı */
STATIC_ASSERT(sizeof(uint32_t) == 4);

uint32_t hdaverb_build(uint32_t nid, uint32_t verb, uint32_t param) {
    return ((nid & 0xFFu) << 20) | ((verb & 0xFFFu) << 8) | (param & 0xFFu);
}

uint32_t hdaverb_nid(uint32_t cmd) {
    return (cmd >> 20) & 0xFFu;
}

uint32_t hdaverb_verb(uint32_t cmd) {
    return (cmd >> 8) & 0xFFFu;
}

uint32_t hdaverb_param(uint32_t cmd) {
    return cmd & 0xFFu;
}
