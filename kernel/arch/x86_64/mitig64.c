#include "arch/x86_64/mitig.h"
#include <string.h>

/* Her guvenlik acigi icin azaltim: teknik bit setleri tablo yoluyla
 * uygulanir. bir acik birden cok teknik ile azaltilabilir; yeterli
 * teknik uygulanmissa MITIGATED sayilir. */

static int  mitig_st[8];
static u32  mitig_tech[8];

static const char *mitig_names[8] = {
    "spectre-v1", "spectre-v2", "meltdown", "retbleed",
    "storebleed", "mds", "l1tf"
};

/* acik -> zorunlu teknikler (hepsinin olmasi sart degil; tam set = guclu) */
static const u32 mitig_need[8] = {
    MITIG64_TECH_SPECRCTRL,              /* spectre-v1: specrctrl */
    MITIG64_TECH_RETPOLINE | MITIG64_TECH_IBRS, /* spectre-v2 */
    MITIG64_TECH_PTI,                    /* meltdown: PTI */
    MITIG64_TECH_IBPB,                   /* retbleed: IBPB */
    MITIG64_TECH_SSBD,                   /* storebleed */
    MITIG64_TECH_MSR_CLR,                /* mds: msr clear */
    MITIG64_TECH_SRDS,                   /* l1tf: srds */
};

int mitig64_init(void) {
    memset(mitig_st, MITIG64_OFF, sizeof(mitig_st));
    memset(mitig_tech, 0, sizeof(mitig_tech));
    return 0;
}

int mitig64_vuln_apply(int vuln_id, u32 tech_bits) {
    if (vuln_id < 0 || vuln_id >= MITIG64__COUNT) return -1;
    if (tech_bits == 0) return -2;
    mitig_tech[vuln_id] |= tech_bits;
    if ((mitig_tech[vuln_id] & mitig_need[vuln_id]) == mitig_need[vuln_id])
        mitig_st[vuln_id] = MITIG64_MITIGATED;
    else
        mitig_st[vuln_id] = MITIG64_PARTIAL;
    return 0;
}

int mitig64_vuln_status(int vuln_id, int *out_status) {
    if (vuln_id < 0 || vuln_id >= MITIG64__COUNT) return -1;
    if (!out_status) return -1;
    *out_status = mitig_st[vuln_id];
    return 0;
}

int mitig64_mitigated_count(void) {
    int i, c = 0;
    for (i = 0; i < MITIG64__COUNT; i++)
        if (mitig_st[i] == MITIG64_MITIGATED) c++;
    return c;
}

int mitig64_unmitigated_count(void) {
    int i, c = 0;
    for (i = 0; i < MITIG64__COUNT; i++)
        if (mitig_st[i] == MITIG64_OFF) c++;
    return c;
}

int mitig64_auto_verify(u32 reported_tech_bits) {
    int i;
    /* rapor sahte olabilir: tek zincir ile butun aciklar kapanmaz */
    if (reported_tech_bits == 0) return 0;
    for (i = 0; i < MITIG64__COUNT; i++) {
        if (mitig_st[i] == MITIG64_MITIGATED &&
            (reported_tech_bits & mitig_tech[i]) == mitig_tech[i])
            continue;
        return 0;
    }
    return 1;
}

int mitig64_cpu_trustworthy(void) {
    if (mitig64_unmitigated_count() > 0) return 0;
    if (mitig64_auto_verify(MITIG64_TECH_SPECRCTRL | MITIG64_TECH_RETPOLINE |
                            MITIG64_TECH_IBRS | MITIG64_TECH_IBPB |
                            MITIG64_TECH_SSBD | MITIG64_TECH_MSR_CLR |
                            MITIG64_TECH_PTI | MITIG64_TECH_SRDS))
        return 1;
    return 0;
}

int mitig64_report(u32 tech_mask, char *buf, int max) {
    int i;
    if (!buf || max <= 0) return -1;
    if (max < 16) return -2;
    buf[0] = 0;
    for (i = 0; i < MITIG64__COUNT; i++) {
        if (tech_mask & (1u << i))
            strncat(buf, mitig_names[i], (size_t)(max - 1) - strlen(buf));
        if (i + 1 < MITIG64__COUNT && (tech_mask & (1u << i)) &&
            (tech_mask & (1u << (i + 1))))
            strncat(buf, ",", (size_t)(max - 1) - strlen(buf));
    }
    return (int)strlen(buf);
}