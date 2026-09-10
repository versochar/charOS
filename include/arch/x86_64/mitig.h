#ifndef MITIG64_H
#define MITIG64_H
#include "arch/x86_64/longmode.h"

/* X86 donanim guvenlik acigi / azaltim modeli */
#define MITIG64_MAX_VULN 16
#define MITIG64_NAME_MAX  24

enum mitig64_vuln {
    MITIG64_SPECTRE_V1 = 0,
    MITIG64_SPECTRE_V2,
    MITIG64_MELTDOWN,
    MITIG64_RETBLEED,
    MITIG64_STOREBLEED,
    MITIG64_MDS,
    MITIG64_L1TF,
    /* acik model kapasitesinin disinda tutulur */
    MITIG64__COUNT,
};

/* azaltim durumu */
#define MITIG64_OFF         0
#define MITIG64_MITIGATED   1
#define MITIG64_PARTIAL     2
#define MITIG64_NOT_AFFECTED 3

/* azaltim teknikleri */
#define MITIG64_TECH_RETPOLINE    0x001u
#define MITIG64_TECH_IBRS         0x002u
#define MITIG64_TECH_IBPB         0x004u
#define MITIG64_TECH_SSBD         0x008u
#define MITIG64_TECH_MSR_CLR      0x010u
#define MITIG64_TECH_PTI          0x020u
#define MITIG64_TECH_SRDS         0x040u
#define MITIG64_TECH_TAA          0x080u
#define MITIG64_TECH_SPECRCTRL    0x100u

int  mitig64_init(void);
int  mitig64_vuln_apply(int vuln_id, u32 tech_bits);
int  mitig64_vuln_status(int vuln_id, int *out_status);
int  mitig64_mitigated_count(void);
int  mitig64_unmitigated_count(void);
int  mitig64_auto_verify(u32 reported_tech_bits);
int  mitig64_cpu_trustworthy(void);
int  mitig64_report(u32 tech_mask, char *buf, int max);

#endif