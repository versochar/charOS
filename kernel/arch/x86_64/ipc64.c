#include "arch/x86_64/longmode.h"
#include "arch/x86_64/ipc.h"

#define IPC_MAX 64

typedef struct {
    u64 chan;
    u64 msg;
    int valid;
    int active;
} ipc_slot_t;

static int ipc_inited = 0;
static u64 chan_ctr = 0;
static ipc_slot_t chans[IPC_MAX];

static ipc_slot_t *find_chan(u64 chan) {
    for (int i = 0; i < IPC_MAX; i++) {
        if (chans[i].active && chans[i].chan == chan) return &chans[i];
    }
    return 0;
}

int ipc64_init(void) {
    ipc_inited = 1;
    chan_ctr = 0;
    for (int i = 0; i < IPC_MAX; i++) {
        chans[i].chan = 0;
        chans[i].msg = 0;
        chans[i].valid = 0;
        chans[i].active = 0;
    }
    return 0;
}

int ipc64_create(u64 *out_chan) {
    if (!ipc_inited || !out_chan) return -1;
    for (int i = 0; i < IPC_MAX; i++) {
        if (!chans[i].active) {
            chan_ctr++;
            chans[i].chan = chan_ctr;
            chans[i].msg = 0;
            chans[i].valid = 0;
            chans[i].active = 1;
            *out_chan = chan_ctr;
            return 0;
        }
    }
    return -1;
}

int ipc64_destroy(u64 chan) {
    ipc_slot_t *s = find_chan(chan);
    if (!s) return -1;
    s->active = 0;
    s->chan = 0;
    s->valid = 0;
    return 0;
}

int ipc64_send(u64 chan, u64 msg) {
    ipc_slot_t *s = find_chan(chan);
    if (!s) return -1;
    if (s->valid) return -1;
    s->msg = msg;
    s->valid = 1;
    return 0;
}

int ipc64_recv(u64 chan, u64 *out_msg) {
    ipc_slot_t *s = find_chan(chan);
    if (!s || !out_msg) return -1;
    if (!s->valid) return -1;
    *out_msg = s->msg;
    s->valid = 0;
    return 0;
}
