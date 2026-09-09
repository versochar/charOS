/* 59G: PXE boot */
#include "arch/x86_64/longmode.h"
#define PXE_MAX 32
struct pxe_entry {int used; int id;};
static struct pxe_entry pxe_tab[PXE_MAX]; static int pxe_next=1;
int pxe64_start(int *out){int i;if(!out)return -1;for(i=0;i<PXE_MAX;i++)if(!pxe_tab[i].used){pxe_tab[i].used=1;pxe_tab[i].id=pxe_next++;*out=pxe_tab[i].id;return 0;}return -2;}
int pxe64_stop(int id){int i;for(i=0;i<PXE_MAX;i++)if(pxe_tab[i].used&&pxe_tab[i].id==id){pxe_tab[i].used=0;return 0;}return -1;}
int pxe64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<PXE_MAX;i++)if(pxe_tab[i].used)c++;*out=c;return 0;}
