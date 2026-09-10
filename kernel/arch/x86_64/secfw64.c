#include "arch/x86_64/longmode.h"
#include "arch/x86_64/secfw.h"

#define SECFW_MAX 16

typedef struct {
    u64 rule;
    int hook;
    u64 subj;
    u64 obj;
    int action;
    int active;
} secfw_rule_t;

static int secfw_inited = 0;
static u64 rule_ctr = 0;
static secfw_rule_t rules[SECFW_MAX];

static secfw_rule_t *find_rule(u64 rule) {
    for (int i = 0; i < SECFW_MAX; i++) {
        if (rules[i].active && rules[i].rule == rule) return &rules[i];
    }
    return 0;
}

int secfw64_init(void) {
    secfw_inited = 1;
    rule_ctr = 0;
    for (int i = 0; i < SECFW_MAX; i++) {
        rules[i].rule = 0; rules[i].active = 0;
    }
    return 0;
}

int secfw64_add(int hook, u64 subj, u64 obj, int action, u64 *out_rule) {
    if (!secfw_inited || !out_rule || hook < 0 || hook > 3 || (action != 0 && action != 1)) return -1;
    for (int i = 0; i < SECFW_MAX; i++) {
        if (!rules[i].active) {
            rule_ctr++;
            rules[i].rule = rule_ctr;
            rules[i].hook = hook;
            rules[i].subj = subj;
            rules[i].obj = obj;
            rules[i].action = action;
            rules[i].active = 1;
            *out_rule = rule_ctr;
            return 0;
        }
    }
    return -1;
}

int secfw64_del(u64 rule) {
    secfw_rule_t *r = find_rule(rule);
    if (!r) return -1;
    r->active = 0; r->rule = 0;
    return 0;
}

int secfw64_check(int hook, u64 subj, u64 obj) {
    if (!secfw_inited) return -1;
    for (int i = 0; i < SECFW_MAX; i++) {
        if (rules[i].active && rules[i].hook == hook && rules[i].subj == subj && rules[i].obj == obj) {
            return rules[i].action ? 0 : -1;
        }
    }
    return -1;
}
