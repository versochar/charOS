#include "arch/x86_64/longmode.h"
#include "arch/x86_64/net.h"

#define NET_MAX 8

typedef struct {
    u64 iface;
    int type;
    u64 pkt;
    int valid;
    u64 tx;
    u64 rx;
    int active;
} net_if_t;

static int net_inited = 0;
static u64 if_ctr = 0;
static net_if_t nics[NET_MAX];

static net_if_t *find_if(u64 iface) {
    for (int i = 0; i < NET_MAX; i++) {
        if (nics[i].active && nics[i].iface == iface) return &nics[i];
    }
    return 0;
}

int net64_init(void) {
    net_inited = 1;
    if_ctr = 0;
    for (int i = 0; i < NET_MAX; i++) {
        nics[i].iface = 0; nics[i].active = 0; nics[i].valid = 0;
        nics[i].tx = 0; nics[i].rx = 0;
    }
    return 0;
}

int net64_if_add(int type, u64 *out_if) {
    if (!net_inited || !out_if || type < 0 || type > 1) return -1;
    for (int i = 0; i < NET_MAX; i++) {
        if (!nics[i].active) {
            if_ctr++;
            nics[i].iface = if_ctr;
            nics[i].type = type;
            nics[i].pkt = 0; nics[i].valid = 0;
            nics[i].tx = 0; nics[i].rx = 0;
            nics[i].active = 1;
            *out_if = if_ctr;
            return 0;
        }
    }
    return -1;
}

int net64_if_del(u64 iface) {
    net_if_t *n = find_if(iface);
    if (!n) return -1;
    n->active = 0; n->iface = 0;
    return 0;
}

int net64_send(u64 iface, u64 val) {
    net_if_t *n = find_if(iface);
    if (!n) return -1;
    if (n->valid) return -1;
    n->pkt = val;
    n->valid = 1;
    n->tx++;
    return 0;
}

int net64_recv(u64 iface, u64 *out_val) {
    net_if_t *n = find_if(iface);
    if (!n || !out_val) return -1;
    if (!n->valid) return -1;
    *out_val = n->pkt;
    n->valid = 0;
    n->rx++;
    return 0;
}

int net64_stat(u64 iface, u64 *out_tx, u64 *out_rx) {
    net_if_t *n = find_if(iface);
    if (!n || !out_tx || !out_rx) return -1;
    *out_tx = n->tx;
    *out_rx = n->rx;
    return 0;
}
