/* 22.3: Gerçek capability mantığı.
 * task yapısına bağımlı DEĞİL: saf bitmask + denetim halkası.
 * Aynı dosya çekirdekte (freestanding) ve host testinde derlenir.
 */
#include "process/cap.h"
#include "core/verify.h"

/* 24.4: derleme-zamanı kanıtları (iki derlemede de denetlenir) */
STATIC_ASSERT((CAP_ALL & 0x1FFFu) == 0x1FFFu);
STATIC_ASSERT(sizeof(struct cap_audit_entry) == 8);

#define CAP_AUDIT_LEN 16

static struct cap_audit_entry audit_ring[CAP_AUDIT_LEN];
static uint32_t audit_pos = 0;

int cap_valid(uint32_t cap) {
    if (cap == 0) return 0;
    if (cap & ~CAP_VALID_MASK) return 0;
    /* tek bit mi */
    return (cap & (cap - 1)) == 0;
}

int cap_has(uint32_t set, uint32_t cap) {
    if (!cap_valid(cap)) return 0;
    return (set & cap) != 0;
}

int cap_grant(uint32_t* set, uint32_t cap) {
    if (REQUIRE(set != 0, 0xC101) != 0) return -1; /* 24.4: sözleşme */
    if (REQUIRE(cap_valid(cap), 0xC102) != 0) return -1;
    *set |= cap;
    return 0;
}

int cap_revoke(uint32_t* set, uint32_t cap) {
    if (REQUIRE(set != 0, 0xC103) != 0) return -1; /* 24.4: sözleşme */
    if (REQUIRE(cap_valid(cap), 0xC104) != 0) return -1;
    *set &= ~cap;
    return 0;
}

int cap_drop_all(uint32_t* set) {
    if (REQUIRE(set != 0, 0xC105) != 0) return -1; /* 24.4: sözleşme */
    *set = 0;
    return 0;
}

int cap_is_subset(uint32_t a, uint32_t b) {
    return (a & ~b) == 0;
}

uint32_t cap_allow_only(uint32_t cur, uint32_t want) {
    if ((want & ~CAP_VALID_MASK) != 0) return cur; /* geçersiz bit: olduğu gibi bırak */
    if (!cap_is_subset(want, cur)) return cur;    /* yükseltme girişimi: reddet */
    return want;
}

void cap_audit(uint32_t cap, uint32_t granted) {
    audit_ring[audit_pos % CAP_AUDIT_LEN].cap = cap;
    audit_ring[audit_pos % CAP_AUDIT_LEN].granted = granted ? 1 : 0;
    audit_pos++;
}

int cap_audit_read(uint32_t idx, uint32_t* cap, uint32_t* granted) {
    if (!cap || !granted) return -1;
    if (idx >= CAP_AUDIT_LEN || idx >= audit_pos) return -1;
    /* idx=0 en yeni kayıt */
    uint32_t pos = (audit_pos - 1 - idx) % CAP_AUDIT_LEN;
    *cap = audit_ring[pos].cap;
    *granted = audit_ring[pos].granted;
    return 0;
}
