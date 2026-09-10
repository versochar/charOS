/* 58H: Flatpak manifest */
#include "arch/x86_64/longmode.h"
#include <string.h>

#define FP_MAX 32
#define FP_RUNTIMES 16

struct fp_entry {int used; int id; char app[64];};
static struct fp_entry fp_tab[FP_MAX]; static int fp_next=1;

/* --- 41H: flatpak runtime kayit tablosu --- */
struct fp_rt {
    char name[64];
    char arch[16];
    char branch[16];
    char ver[16];
    int  used;
};
static struct fp_rt fp_rts[FP_RUNTIMES];

int flatpak64_parse(const char *manifest,int *out){int i;if(!manifest||!out)return -1;for(i=0;i<FP_MAX;i++)if(!fp_tab[i].used){fp_tab[i].used=1;fp_tab[i].id=fp_next++;*out=fp_tab[i].id;return 0;}return -2;}
int flatpak64_remove(int id){int i;for(i=0;i<FP_MAX;i++)if(fp_tab[i].used&&fp_tab[i].id==id){fp_tab[i].used=0;return 0;}return -1;}
int flatpak64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<FP_MAX;i++)if(fp_tab[i].used)c++;*out=c;return 0;}

int flatpak64_parse_ref(const char *s, struct flatpak64_ref *out) {
    const char *p, *seg[4];
    int n = 0;
    size_t l;
    if (!s || !out) return -1;
    memset(out, 0, sizeof(*out));
    p = s;
    while (*p && n < 4) {
        seg[n++] = p;
        p = strchr(p, '/');
        if (!p) break;
        p++;
    }
    if (n < 3) return -2; /* name/arch/branch en az 3 parca */

    /* istege bagli "runtime/" (veya "app/") oneki */
    if (n == 4) {
        l = (size_t)(seg[1] - seg[0] - 1);
        if (l >= 16) return -2;
        memcpy(out->kind, seg[0], l);
        seg[0] = seg[1];
        seg[1] = seg[2];
        seg[2] = seg[3];
        n--;
    } else {
        memcpy(out->kind, "app", 4);
    }
    l = (size_t)(seg[1] - seg[0] - 1);
    if (l >= 64) return -2;
    memcpy(out->name, seg[0], l);
    l = (size_t)(seg[2] - seg[1] - 1);
    if (l >= 16) return -2;
    memcpy(out->arch, seg[1], l);
    if (n == 3) {
        l = strlen(seg[2]);
        if (l >= 16) return -2;
        memcpy(out->branch, seg[2], l);
    }
    return 0;
}

int flatpak64_add_runtime(const char *name, const char *arch,
                          const char *branch, const char *ver) {
    int i;
    if (!name || !arch || !branch || !ver) return -1;
    for (i = 0; i < FP_RUNTIMES; i++) {
        if (fp_rts[i].used && strcmp(fp_rts[i].name, name) == 0 &&
            strcmp(fp_rts[i].arch, arch) == 0 &&
            strcmp(fp_rts[i].branch, branch) == 0)
            return -2; /* zaten kayitli */
    }
    for (i = 0; i < FP_RUNTIMES; i++) {
        if (!fp_rts[i].used) {
            strncpy(fp_rts[i].name, name, sizeof(fp_rts[i].name) - 1);
            strncpy(fp_rts[i].arch, arch, sizeof(fp_rts[i].arch) - 1);
            strncpy(fp_rts[i].branch, branch, sizeof(fp_rts[i].branch) - 1);
            strncpy(fp_rts[i].ver, ver, sizeof(fp_rts[i].ver) - 1);
            fp_rts[i].used = 1;
            return 0;
        }
    }
    return -3;
}

int flatpak64_find_runtime(const char *name, const char *arch,
                           const char *branch, char *ver_out, int max) {
    int i;
    if (!name || !arch || !branch) return -1;
    for (i = 0; i < FP_RUNTIMES; i++) {
        if (fp_rts[i].used && strcmp(fp_rts[i].name, name) == 0 &&
            strcmp(fp_rts[i].arch, arch) == 0 &&
            strcmp(fp_rts[i].branch, branch) == 0) {
            if (ver_out) {
                if (max <= 0) return -2;
                strncpy(ver_out, fp_rts[i].ver, (size_t)max - 1);
                ver_out[max - 1] = '\0';
            }
            return 0;
        }
    }
    return -3;
}