/* 59A: Live ISO 64-bit */
#include "arch/x86_64/longmode.h"
#define LI_MAX 32
struct li_entry {int used; int id; char path[128];};
static struct li_entry li_tab[LI_MAX]; static int li_next=1;
int liveiso64_create(const char *path,int *out){int i;if(!path||!out)return -1;for(i=0;i<LI_MAX;i++)if(!li_tab[i].used){li_tab[i].used=1;li_tab[i].id=li_next++;*out=li_tab[i].id;return 0;}return -2;}
int liveiso64_destroy(int id){int i;for(i=0;i<LI_MAX;i++)if(li_tab[i].used&&li_tab[i].id==id){li_tab[i].used=0;return 0;}return -1;}
int liveiso64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<LI_MAX;i++)if(li_tab[i].used)c++;*out=c;return 0;}
