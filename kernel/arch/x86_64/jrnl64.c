#include "arch/x86_64/longmode.h"
#include "arch/x86_64/jrnl.h"

#define JRNL_MAX 16
#define JRNL_LOG 8

#define JRNL_FREE 0
#define JRNL_ACTIVE 1
#define JRNL_COMMITTED 2
#define JRNL_ABORTED 3

typedef struct {
    u64 tx;
    int state;
    u64 log[JRNL_LOG];
    u64 n;
} jrnl_slot_t;

static int jrnl_inited = 0;
static u64 tx_ctr = 0;
static u64 committed = 0;
static jrnl_slot_t txs[JRNL_MAX];

static jrnl_slot_t *find_tx(u64 tx) {
    for (int i = 0; i < JRNL_MAX; i++) {
        if (txs[i].state != JRNL_FREE && txs[i].tx == tx) return &txs[i];
    }
    return 0;
}

int jrnl64_init(void) {
    jrnl_inited = 1;
    tx_ctr = 0; committed = 0;
    for (int i = 0; i < JRNL_MAX; i++) { txs[i].tx = 0; txs[i].state = JRNL_FREE; txs[i].n = 0; }
    return 0;
}

int jrnl64_begin(u64 *out_tx) {
    if (!jrnl_inited || !out_tx) return -1;
    for (int i = 0; i < JRNL_MAX; i++) {
        if (txs[i].state == JRNL_FREE) {
            tx_ctr++;
            txs[i].tx = tx_ctr;
            txs[i].state = JRNL_ACTIVE;
            txs[i].n = 0;
            *out_tx = tx_ctr;
            return 0;
        }
    }
    return -1;
}

int jrnl64_append(u64 tx, u64 val) {
    jrnl_slot_t *s = find_tx(tx);
    if (!s) return -1;
    if (s->state != JRNL_ACTIVE) return -1;
    if (s->n >= JRNL_LOG) return -2;
    s->log[s->n++] = val;
    return 0;
}

int jrnl64_commit(u64 tx) {
    jrnl_slot_t *s = find_tx(tx);
    if (!s) return -1;
    if (s->state != JRNL_ACTIVE) return -1;
    s->state = JRNL_COMMITTED;
    committed++;
    return 0;
}

int jrnl64_abort(u64 tx) {
    jrnl_slot_t *s = find_tx(tx);
    if (!s) return -1;
    if (s->state != JRNL_ACTIVE) return -1;
    s->state = JRNL_ABORTED;
    return 0;
}

int jrnl64_replay(u64 *out_count) {
    if (!jrnl_inited || !out_count) return -1;
    *out_count = committed;
    committed = 0;
    for (int i = 0; i < JRNL_MAX; i++) {
        if (txs[i].state == JRNL_COMMITTED || txs[i].state == JRNL_ABORTED) {
            txs[i].state = JRNL_FREE; txs[i].tx = 0;
        }
    }
    return 0;
}
