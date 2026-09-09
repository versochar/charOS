/* 58F: cgroups v2 */
#include "arch/x86_64/longmode.h"
#define CG_MAX 32
struct cg_entry {int used; int id; char name[64];};
static struct cg_entry cg_tab[CG_MAX];
static int cg_next=1;
int cgroup64_create(const char *name,int *out){int i;if(!name||!out)return -1;for(i=0;i<CG_MAX;i++)if(!cg_tab[i].used){cg_tab[i].used=1;cg_tab[i].id=cg_next++;/* copy name */*out=cg_tab[i].id;return 0;}return -2;}
int cgroup64_destroy(int id){int i;for(i=0;i<CG_MAX;i++)if(cg_tab[i].used&&cg_tab[i].id==id){cg_tab[i].used=0;return 0;}return -1;}
int cgroup64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<CG_MAX;i++)if(cg_tab[i].used)c++;*out=c;return 0;}
