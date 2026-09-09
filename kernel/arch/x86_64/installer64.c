/* 59B: Kurulum sihirbazı */
#include "arch/x86_64/longmode.h"
#define INS_MAX 32
struct ins_entry {int used; int id;};
static struct ins_entry ins_tab[INS_MAX]; static int ins_next=1;
int installer64_start(int *out){int i;if(!out)return -1;for(i=0;i<INS_MAX;i++)if(!ins_tab[i].used){ins_tab[i].used=1;ins_tab[i].id=ins_next++;*out=ins_tab[i].id;return 0;}return -2;}
int installer64_stop(int id){int i;for(i=0;i<INS_MAX;i++)if(ins_tab[i].used&&ins_tab[i].id==id){ins_tab[i].used=0;return 0;}return -1;}
int installer64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<INS_MAX;i++)if(ins_tab[i].used)c++;*out=c;return 0;}
