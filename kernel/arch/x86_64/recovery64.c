/* 59I: Recovery ISO */
#include "arch/x86_64/longmode.h"
#define RC_MAX 32
struct rc_entry {int used; int id;};
static struct rc_entry rc_tab[RC_MAX]; static int rc_next=1;
int recovery64_create(int *out){int i;if(!out)return -1;for(i=0;i<RC_MAX;i++)if(!rc_tab[i].used){rc_tab[i].used=1;rc_tab[i].id=rc_next++;*out=rc_tab[i].id;return 0;}return -2;}
int recovery64_destroy(int id){int i;for(i=0;i<RC_MAX;i++)if(rc_tab[i].used&&rc_tab[i].id==id){rc_tab[i].used=0;return 0;}return -1;}
int recovery64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<RC_MAX;i++)if(rc_tab[i].used)c++;*out=c;return 0;}
