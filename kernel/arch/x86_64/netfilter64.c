/* 43I: netfilter — zincir + sira-oncelikli kural esleme. */
#include "arch/x86_64/longmode.h"

#define NETFILTER64_MAX_RULES 64
#define NETFILTER64_CHAINS 3 /* 0=INPUT,1=OUTPUT,2=FORWARD */

struct netfilter64_rule {
    int used;
    int chain;
    u32 src;
    u32 smask;
    u32 dst;
    u32 dmask;
    u8 proto;   /* 0 = hepsi */
    u16 dport;  /* 0 = hepsi */
    int verdict;
};

static struct netfilter64_rule netfilter64_tab[NETFILTER64_MAX_RULES];

int netfilter64_add(int chain, u32 src, u32 smask, u32 dst, u32 dmask,
                    u8 proto, u16 dport, int verdict) {
    int i;
    if (chain < 0 || chain >= NETFILTER64_CHAINS) return -1;
    if (verdict != NETFILTER64_ACCEPT && verdict != NETFILTER64_DROP &&
        verdict != NETFILTER64_LOG)
        return -1;
    for (i = 0; i < NETFILTER64_MAX_RULES; i++) {
        if (!netfilter64_tab[i].used) {
            netfilter64_tab[i].used = 1;
            netfilter64_tab[i].chain = chain;
            netfilter64_tab[i].src = src;
            netfilter64_tab[i].smask = smask;
            netfilter64_tab[i].dst = dst;
            netfilter64_tab[i].dmask = dmask;
            netfilter64_tab[i].proto = proto;
            netfilter64_tab[i].dport = dport;
            netfilter64_tab[i].verdict = verdict;
            return 0;
        }
    }
    return -2;
}

int netfilter64_hook(int chain, const struct netfilter64_pkt *pkt) {
    int i, logged = 0, verdict = NETFILTER64_ACCEPT;
    if (!pkt || chain < 0 || chain >= NETFILTER64_CHAINS) return -1;
    for (i = 0; i < NETFILTER64_MAX_RULES; i++) {
        const struct netfilter64_rule *r = &netfilter64_tab[i];
        if (!r->used || r->chain != chain) continue;
        if ((pkt->src & r->smask) != (r->src & r->smask)) continue;
        if ((pkt->dst & r->dmask) != (r->dst & r->dmask)) continue;
        if (r->proto && r->proto != pkt->proto) continue;
        if (r->dport && r->dport != pkt->dport) continue;
        if (r->verdict == NETFILTER64_LOG) {
            logged = 1;
            continue;
        }
        verdict = r->verdict;
        break; /* ilk kesin karar kazanir */
    }
    (void)logged;
    return verdict;
}
