/* 59H: Auto-install */
#include "arch/x86_64/longmode.h"
#define AI_MAX 32
struct ai_entry {int used; int id;};
static struct ai_entry ai_tab[AI_MAX]; static int ai_next=1;
int autoinstall64_start(int *out){int i;if(!out)return -1;for(i=0;i<AI_MAX;i++)if(!ai_tab[i].used){ai_tab[i].used=1;ai_tab[i].id=ai_next++;*out=ai_tab[i].id;return 0;}return -2;}
int autoinstall64_stop(int id){int i;for(i=0;i<AI_MAX;i++)if(ai_tab[i].used&&ai_tab[i].id==id){ai_tab[i].used=0;return 0;}return -1;}
int autoinstall64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<AI_MAX;i++)if(ai_tab[i].used)c++;*out=c;return 0;}
