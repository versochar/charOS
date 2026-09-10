#include "arch/x86_64/longmode.h"
#include "arch/x86_64/sync.h"

#define SYNC_MAX 32

typedef struct {
    u64 id;
    int locked;
    int active;
} spin_slot_t;

typedef struct {
    u64 id;
    int locked;
    u64 owner;
    int active;
} mutex_slot_t;

static int sync_inited = 0;
static u64 sync_ctr = 0;
static spin_slot_t spins[SYNC_MAX];
static mutex_slot_t mutexes[SYNC_MAX];

static spin_slot_t *find_spin(u64 id) {
    for (int i = 0; i < SYNC_MAX; i++) {
        if (spins[i].active && spins[i].id == id) return &spins[i];
    }
    return 0;
}

static mutex_slot_t *find_mutex(u64 id) {
    for (int i = 0; i < SYNC_MAX; i++) {
        if (mutexes[i].active && mutexes[i].id == id) return &mutexes[i];
    }
    return 0;
}

int sync64_init(void) {
    sync_inited = 1;
    sync_ctr = 0;
    for (int i = 0; i < SYNC_MAX; i++) {
        spins[i].id = 0; spins[i].locked = 0; spins[i].active = 0;
        mutexes[i].id = 0; mutexes[i].locked = 0; mutexes[i].owner = 0; mutexes[i].active = 0;
    }
    return 0;
}

int spin64_create(u64 *out_id) {
    if (!sync_inited || !out_id) return -1;
    for (int i = 0; i < SYNC_MAX; i++) {
        if (!spins[i].active) {
            sync_ctr++;
            spins[i].id = sync_ctr;
            spins[i].locked = 0;
            spins[i].active = 1;
            *out_id = sync_ctr;
            return 0;
        }
    }
    return -1;
}

int spin64_trylock(u64 id) {
    spin_slot_t *s = find_spin(id);
    if (!s) return -1;
    if (s->locked) return -1;
    s->locked = 1;
    return 0;
}

int spin64_unlock(u64 id) {
    spin_slot_t *s = find_spin(id);
    if (!s) return -1;
    if (!s->locked) return -1;
    s->locked = 0;
    return 0;
}

int spin64_destroy(u64 id) {
    spin_slot_t *s = find_spin(id);
    if (!s) return -1;
    s->active = 0; s->id = 0; s->locked = 0;
    return 0;
}

int mutex64_create(u64 *out_id) {
    if (!sync_inited || !out_id) return -1;
    for (int i = 0; i < SYNC_MAX; i++) {
        if (!mutexes[i].active) {
            sync_ctr++;
            mutexes[i].id = sync_ctr;
            mutexes[i].locked = 0;
            mutexes[i].owner = 0;
            mutexes[i].active = 1;
            *out_id = sync_ctr;
            return 0;
        }
    }
    return -1;
}

int mutex64_lock(u64 id, u64 owner) {
    mutex_slot_t *m = find_mutex(id);
    if (!m || owner == 0) return -1;
    if (m->locked) return -1;
    m->locked = 1;
    m->owner = owner;
    return 0;
}

int mutex64_unlock(u64 id, u64 owner) {
    mutex_slot_t *m = find_mutex(id);
    if (!m) return -1;
    if (!m->locked) return -1;
    if (m->owner != owner) return -1;
    m->locked = 0;
    m->owner = 0;
    return 0;
}

int mutex64_destroy(u64 id) {
    mutex_slot_t *m = find_mutex(id);
    if (!m) return -1;
    m->active = 0; m->id = 0; m->locked = 0; m->owner = 0;
    return 0;
}
