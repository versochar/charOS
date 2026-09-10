#include "arch/x86_64/longmode.h"
#include "arch/x86_64/netsec.h"

#define NETSEC_MAX 16

typedef struct {
    u64 rule;
    u64 addr;
    u64 port;
    int action;
    int active;
} netsec_rule_t;

static int netsec_inited = 0;
static u64 rule_ctr = 0;
static netsec_rule_t rules[NETSEC_MAX];

static netsec_rule_t *find_rule(u64 rule) {
    for (int i = 0; i < NETSEC_MAX; i++) {
        if (rules[i].active && rules[i].rule == rule) return &rules[i];
    }
    return 0;
}

int netsec64_init(void) {
    netsec_inited = 1;
    rule_ctr = 0;
    for (int i = 0; i < NETSEC_MAX; i++) {
        rules[i].rule = 0; rules[i].active = 0;
    }
    return 0;
}

int netsec64_add(u64 addr, u64 port, int action, u64 *out_rule) {
    if (!netsec_inited || !out_rule || (action != 0 && action != 1)) return -1;
    for (int i = 0; i < NETSEC_MAX; i++) {
        if (!rules[i].active) {
            rule_ctr++;
            rules[i].rule = rule_ctr;
            rules[i].addr = addr;
            rules[i].port = port;
            rules[i].action = action;
            rules[i].active = 1;
            *out_rule = rule_ctr;
            return 0;
        }
    }
    return -1;
}

int netsec64_del(u64 rule) {
    netsec_rule_t *r = find_rule(rule);
    if (!r) return -1;
    r->active = 0; r->rule = 0;
    return 0;
}

int netsec64_check(u64 addr, u64 port) {
    if (!netsec_inited) return -1;
    for (int i = 0; i < NETSEC_MAX; i++) {
        if (rules[i].active && rules[i].addr == addr && rules[i].port == port) {
            return rules[i].action ? 0 : -1;
        }
    }
    return -1;
}
