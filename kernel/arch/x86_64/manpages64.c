/* 60C */
#include "arch/x86_64/longmode.h"
#define MAX 32
struct entry {int used; int id;};
static struct entry tab[MAX]; static int next=1;
int manpages64_create(int *out){int i;if(!out)return -1;for(i=0;i<MAX;i++)if(!tab[i].used){tab[i].used=1;tab[i].id=next++;*out=tab[i].id;return 0;}return -2;}
int manpages64_destroy(int id){int i;for(i=0;i<MAX;i++)if(tab[i].used&&tab[i].id==id){tab[i].used=0;return 0;}return -1;}
int manpages64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<MAX;i++)if(tab[i].used)c++;*out=c;return 0;}
