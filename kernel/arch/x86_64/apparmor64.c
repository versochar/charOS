/* 58G: AppArmor profil */
#include "arch/x86_64/longmode.h"
#define AA_MAX 32
struct aa_entry {int used; int id; char name[64];};
static struct aa_entry aa_tab[AA_MAX]; static int aa_next=1;
int apparmor64_load(const char *name,int *out){int i;if(!name||!out)return -1;for(i=0;i<AA_MAX;i++)if(!aa_tab[i].used){aa_tab[i].used=1;aa_tab[i].id=aa_next++;*out=aa_tab[i].id;return 0;}return -2;}
int apparmor64_unload(int id){int i;for(i=0;i<AA_MAX;i++)if(aa_tab[i].used&&aa_tab[i].id==id){aa_tab[i].used=0;return 0;}return -1;}
int apparmor64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<AA_MAX;i++)if(aa_tab[i].used)c++;*out=c;return 0;}
