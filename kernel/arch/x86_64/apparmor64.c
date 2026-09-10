/* 58G: AppArmor profil */
#include "arch/x86_64/longmode.h"
#include <string.h>

#define AA_MAX 32
#define AA_RULES_MAX 32

struct aa_entry {int used; int id; char name[64];};
static struct aa_entry aa_tab[AA_MAX]; static int aa_next=1;

int apparmor64_load(const char *name,int *out){int i;if(!name||!out)return -1;for(i=0;i<AA_MAX;i++)if(!aa_tab[i].used){aa_tab[i].used=1;aa_tab[i].id=aa_next++;*out=aa_tab[i].id;return 0;}return -2;}
int apparmor64_unload(int id){int i;for(i=0;i<AA_MAX;i++)if(aa_tab[i].used&&aa_tab[i].id==id){aa_tab[i].used=0;return 0;}return -1;}
int apparmor64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<AA_MAX;i++)if(aa_tab[i].used)c++;*out=c;return 0;}

/* --- 37G: AppArmor kural motoru --- */
#define AA_PREFIX_MAX 64
struct aa_profile {
    char prefix[AA_PREFIX_MAX];
    int  used;
};
static struct aa_profile aa_profiles[AA_MAX];
static int aa_profiles_n = 0;

struct aa_rule {
    char prefix[AA_PREFIX_MAX];
    char path[AA_PREFIX_MAX];
    int  op;
    int  used;
};
static struct aa_rule aa_rules[AA_RULES_MAX];

static int aa_has_prefix(const char *prefix, const char *path) {
    size_t n = strlen(prefix);
    return strncmp(prefix, path, n) == 0;
}

int apparmor64_add_profile(const char *prefix, int enforce) {
    int i;
    (void)enforce;
    if (!prefix || !prefix[0]) return -1;
    for (i = 0; i < AA_MAX; i++)
        if (aa_profiles[i].used && strcmp(aa_profiles[i].prefix, prefix) == 0)
            return 0;
    for (i = 0; i < AA_MAX; i++) {
        if (!aa_profiles[i].used) {
            strncpy(aa_profiles[i].prefix, prefix,
                    sizeof(aa_profiles[i].prefix) - 1);
            aa_profiles[i].used = 1;
            aa_profiles_n++;
            return 0;
        }
    }
    return -2;
}

int apparmor64_add_rule(const char *prefix, const char *path, int op) {
    int i;
    if (!prefix || !path || op < APPARMOR64_R || op > APPARMOR64_X)
        return -1;
    for (i = 0; i < AA_RULES_MAX; i++) {
        if (!aa_rules[i].used) {
            strncpy(aa_rules[i].prefix, prefix,
                    sizeof(aa_rules[i].prefix) - 1);
            strncpy(aa_rules[i].path, path,
                    sizeof(aa_rules[i].path) - 1);
            aa_rules[i].op = op;
            aa_rules[i].used = 1;
            return 0;
        }
    }
    return -2;
}

int apparmor64_check(const char *path, int op) {
    int i;
    int profiled = 0;
    if (!path) return -1;
    if (op < APPARMOR64_R || op > APPARMOR64_X) return -1;
    /* hangi profil altindayiz? */
    for (i = 0; i < AA_MAX; i++) {
        if (aa_profiles[i].used && aa_has_prefix(aa_profiles[i].prefix, path)) {
            profiled = 1;
            break;
        }
    }
    if (!profiled) return 1; /* profilsiz dosya serbest */
    for (i = 0; i < AA_RULES_MAX; i++) {
        if (aa_rules[i].used &&
            aa_has_prefix(aa_rules[i].prefix, path) &&
            strcmp(aa_rules[i].path, path) == 0 &&
            aa_rules[i].op == op)
            return 1;
    }
    return 0; /* izin verilmemis */
}