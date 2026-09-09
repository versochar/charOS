/* 59E: Bootloader */
#include "arch/x86_64/longmode.h"
#define BL_MAX 32
struct bl_entry {int used; int id;};
static struct bl_entry bl_tab[BL_MAX]; static int bl_next=1;
int bootloader64_install(int *out){int i;if(!out)return -1;for(i=0;i<BL_MAX;i++)if(!bl_tab[i].used){bl_tab[i].used=1;bl_tab[i].id=bl_next++;*out=bl_tab[i].id;return 0;}return -2;}
int bootloader64_remove(int id){int i;for(i=0;i<BL_MAX;i++)if(bl_tab[i].used&&bl_tab[i].id==id){bl_tab[i].used=0;return 0;}return -1;}
int bootloader64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<BL_MAX;i++)if(bl_tab[i].used)c++;*out=c;return 0;}
