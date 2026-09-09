/* 60B: Changelog */
#include "arch/x86_64/longmode.h"
#define CL_MAX 32
struct cl_entry {int used; int id;};
static struct cl_entry cl_tab[CL_MAX]; static int cl_next=1;
int changelog64_create(int *out){int i;if(!out)return -1;for(i=0;i<CL_MAX;i++)if(!cl_tab[i].used){cl_tab[i].used=1;cl_tab[i].id=cl_next++;*out=cl_tab[i].id;return 0;}return -2;}
int changelog64_destroy(int id){int i;for(i=0;i<CL_MAX;i++)if(cl_tab[i].used&&cl_tab[i].id==id){cl_tab[i].used=0;return 0;}return -1;}
int changelog64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<CL_MAX;i++)if(cl_tab[i].used)c++;*out=c;return 0;}
