/* 58I: runtime izolasyon */
#include "arch/x86_64/longmode.h"
#define RT_MAX 32
struct rt_entry {int used; int id; char name[64];};
static struct rt_entry rt_tab[RT_MAX]; static int rt_next=1;
int runtime64_start(const char *name,int *out){int i;if(!name||!out)return -1;for(i=0;i<RT_MAX;i++)if(!rt_tab[i].used){rt_tab[i].used=1;rt_tab[i].id=rt_next++;*out=rt_tab[i].id;return 0;}return -2;}
int runtime64_stop(int id){int i;for(i=0;i<RT_MAX;i++)if(rt_tab[i].used&&rt_tab[i].id==id){rt_tab[i].used=0;return 0;}return -1;}
int runtime64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<RT_MAX;i++)if(rt_tab[i].used)c++;*out=c;return 0;}
