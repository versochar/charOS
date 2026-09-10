#include "arch/x86_64/longmode.h"
#include "arch/x86_64/netadv.h"

#define NETADV_MAX 16

typedef struct {
    u64 route;
    u64 prefix;
    u64 mask;
    u64 gw;
    int active;
} netadv_entry_t;

static int netadv_inited = 0;
static u64 route_ctr = 0;
static netadv_entry_t routes[NETADV_MAX];

static netadv_entry_t *find_route(u64 route) {
    for (int i = 0; i < NETADV_MAX; i++) {
        if (routes[i].active && routes[i].route == route) return &routes[i];
    }
    return 0;
}

int netadv64_init(void) {
    netadv_inited = 1;
    route_ctr = 0;
    for (int i = 0; i < NETADV_MAX; i++) {
        routes[i].route = 0; routes[i].active = 0;
    }
    return 0;
}

int netadv64_add(u64 prefix, u64 mask, u64 gw, u64 *out_route) {
    if (!netadv_inited || !out_route) return -1;
    for (int i = 0; i < NETADV_MAX; i++) {
        if (!routes[i].active) {
            route_ctr++;
            routes[i].route = route_ctr;
            routes[i].prefix = prefix;
            routes[i].mask = mask;
            routes[i].gw = gw;
            routes[i].active = 1;
            *out_route = route_ctr;
            return 0;
        }
    }
    return -1;
}

int netadv64_del(u64 route) {
    netadv_entry_t *r = find_route(route);
    if (!r) return -1;
    r->active = 0; r->route = 0;
    return 0;
}

int netadv64_lookup(u64 addr, u64 *out_gw) {
    if (!netadv_inited || !out_gw) return -1;
    for (int i = 0; i < NETADV_MAX; i++) {
        if (routes[i].active && ((addr & routes[i].mask) == routes[i].prefix)) {
            *out_gw = routes[i].gw;
            return 0;
        }
    }
    return -1;
}
