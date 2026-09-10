#include "arch/x86_64/luksop.h"
#include <string.h>

struct luks_slot {
    u8  used;
    u64 kdf_iter;
    u64 key_ref;   /* anahtar malzemesi ozeti (kayitli) */
    u64 crc;       /* slot dogrulama kontrolu */
};
struct luks_vol {
    u64  dev;
    u64  magic;
    char cipher[LUKSOP64_CIPHER_MAX];
    int  key_bits;
    struct luks_slot slots[LUKSOP64_SLOTS];
    int  state;
    u64  crc;
};
static struct luks_vol vol;
static int vol_inited = 0;

static u64 luks_hash(u64 a, u64 b) {
    u64 h = 1469598103934665603ULL ^ a;
    h ^= b;
    h *= 1099511628211ULL;
    h ^= b >> 17;
    h *= 1099511628211ULL;
    return h ? h : 1;
}

static u64 slot_key(const struct luks_slot *s) {
    return luks_hash(s->kdf_iter, s->key_ref);
}

static void vol_crc_recalc(void) {
    int i;
    u64 h = vol.magic ^ (u64)vol.key_bits;
    h ^= (u64)vol.state;
    for (i = 0; i < LUKSOP64_SLOTS; i++) {
        if (vol.slots[i].used) h ^= slot_key(&vol.slots[i]);
        h *= 1099511628211ULL;
    }
    vol.crc = h ? h : 1;
}

int luksop64_init(void) {
    memset(&vol, 0, sizeof(vol));
    vol.magic = LUKSOP64_MAGIC;
    vol.state = LUKSOP_CLOSED;
    vol_inited = 1;
    vol_crc_recalc();
    return 0;
}

int luksop64_format(u64 device, const char *cipher, int key_bits) {
    if (!cipher) return -1;
    if (vol_inited && vol.state == LUKSOP_UNLOCKED) return -2;
    if (key_bits != 128 && key_bits != 256 && key_bits != 512) return -3;
    memset(&vol, 0, sizeof(vol));
    vol.dev = device;
    vol.magic = LUKSOP64_MAGIC;
    vol.key_bits = key_bits;
    vol.state = LUKSOP_CLOSED;
    strncpy(vol.cipher, cipher, sizeof(vol.cipher) - 1);
    vol_inited = 1;
    vol_crc_recalc();
    return 0;
}

int luksop64_add_key_slot(int slot, u64 kdf_iter, u64 key_material) {
    struct luks_slot *s;
    if (slot < 0 || slot >= LUKSOP64_SLOTS) return -1;
    if (!vol_inited || vol.magic != LUKSOP64_MAGIC) return -2;
    if (vol.state == LUKSOP_UNLOCKED) return -3;
    if (kdf_iter < 1000) return -4; /* kdf tekrari cok dusuk */
    if (!key_material) return -5;
    if (vol.slots[slot].used) return -6; /* slot dolu */
    s = &vol.slots[slot];
    s->used = 1;
    s->kdf_iter = kdf_iter;
    s->key_ref = luks_hash(kdf_iter, key_material);
    s->crc = slot_key(s);
    vol_crc_recalc();
    return 0;
}

int luksop64_verify_key(int slot, u64 key_material) {
    struct luks_slot *s;
    u64 ref;
    if (slot < 0 || slot >= LUKSOP64_SLOTS) return -1;
    if (!vol.slots[slot].used) return -2;
    s = &vol.slots[slot];
    ref = luks_hash(s->kdf_iter, key_material);
    return ref == s->key_ref ? 0 : -3;
}

int luksop64_unlock(int slot, u64 key_material) {
    if (vol.state == LUKSOP_UNLOCKED) return -3; /* zaten acik */
    if (luksop64_verify_key(slot, key_material) != 0) return -4;
    vol.state = LUKSOP_UNLOCKED;
    vol_crc_recalc();
    return 0;
}

int luksop64_lock(void) {
    if (vol.state == LUKSOP_CLOSED) return -1;
    vol.state = LUKSOP_CLOSED;
    vol_crc_recalc();
    return 0;
}

int luksop64_state(int *out) {
    if (!out) return -1;
    *out = vol.state;
    return 0;
}

int luksop64_header_crc(u64 device) {
    if (!vol_inited || vol.dev != device) return 0;
    return (int)(vol.crc & 0x7FFFFFFF);
}