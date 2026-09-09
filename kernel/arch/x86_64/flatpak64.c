/* 58H: Flatpak manifest */
#include "arch/x86_64/longmode.h"
#define FP_MAX 32
struct fp_entry {int used; int id; char app[64];};
static struct fp_entry fp_tab[FP_MAX]; static int fp_next=1;
int flatpak64_parse(const char *manifest,int *out){int i;if(!manifest||!out)return -1;for(i=0;i<FP_MAX;i++)if(!fp_tab[i].used){fp_tab[i].used=1;fp_tab[i].id=fp_next++;*out=fp_tab[i].id;return 0;}return -2;}
int flatpak64_remove(int id){int i;for(i=0;i<FP_MAX;i++)if(fp_tab[i].used&&fp_tab[i].id==id){fp_tab[i].used=0;return 0;}return -1;}
int flatpak64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<FP_MAX;i++)if(fp_tab[i].used)c++;*out=c;return 0;}
