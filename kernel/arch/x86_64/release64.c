/* 60A: Release engineering */
#include "arch/x86_64/longmode.h"
#define REL_MAX 32
struct rel_entry {int used; int id;};
static struct rel_entry rel_tab[REL_MAX]; static int rel_next=1;
int release64_create(int *out){int i;if(!out)return -1;for(i=0;i<REL_MAX;i++)if(!rel_tab[i].used){rel_tab[i].used=1;rel_tab[i].id=rel_next++;*out=rel_tab[i].id;return 0;}return -2;}
int release64_destroy(int id){int i;for(i=0;i<REL_MAX;i++)if(rel_tab[i].used&&rel_tab[i].id==id){rel_tab[i].used=0;return 0;}return -1;}
int release64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<REL_MAX;i++)if(rel_tab[i].used)c++;*out=c;return 0;}
