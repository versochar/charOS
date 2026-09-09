/* 59C: Partition wizard */
#include "arch/x86_64/longmode.h"
#define PT_MAX 32
struct pt_entry {int used; int id;};
static struct pt_entry pt_tab[PT_MAX]; static int pt_next=1;
int partition64_new(int *out){int i;if(!out)return -1;for(i=0;i<PT_MAX;i++)if(!pt_tab[i].used){pt_tab[i].used=1;pt_tab[i].id=pt_next++;*out=pt_tab[i].id;return 0;}return -2;}
int partition64_del(int id){int i;for(i=0;i<PT_MAX;i++)if(pt_tab[i].used&&pt_tab[i].id==id){pt_tab[i].used=0;return 0;}return -1;}
int partition64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<PT_MAX;i++)if(pt_tab[i].used)c++;*out=c;return 0;}
