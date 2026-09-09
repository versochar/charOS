/* 59F: Network install */
#include "arch/x86_64/longmode.h"
#define NI_MAX 32
struct ni_entry {int used; int id;};
static struct ni_entry ni_tab[NI_MAX]; static int ni_next=1;
int netinstall64_start(int *out){int i;if(!out)return -1;for(i=0;i<NI_MAX;i++)if(!ni_tab[i].used){ni_tab[i].used=1;ni_tab[i].id=ni_next++;*out=ni_tab[i].id;return 0;}return -2;}
int netinstall64_stop(int id){int i;for(i=0;i<NI_MAX;i++)if(ni_tab[i].used&&ni_tab[i].id==id){ni_tab[i].used=0;return 0;}return -1;}
int netinstall64_count(int *out){int i,c=0;if(!out)return -1;for(i=0;i<NI_MAX;i++)if(ni_tab[i].used)c++;*out=c;return 0;}
