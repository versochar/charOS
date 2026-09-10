#ifndef RECENV64_H
#define RECENV64_H
#include "arch/x86_64/longmode.h"

#define RECENV64_MODES    4
#define RECENV64_NAME_MAX 24

enum recenv64_state {
    RECENV_NONE = 0,
    RECENV_MENU = 1,
    RECENV_RESOLVED = 2,
};

/* onerilen modlar / donguler */
#define RECENV_MODE_SAFE    0
#define RECENV_MODE_RESCUE  1
#define RECENV_MODE_RESTORE 2
#define RECENV_MODE_NETBOOT 3
#define RECENV_MODES_N      4

int recenv64_init(void);
int recenv64_boot_failed(void);          /* artis + geri sayim */
int recenv64_menu_timeout(int seconds);  /* 0 = sonsuz; 1..300 sec */
int recenv64_enter(void);                 /* RECENV_NONE -> RECENV_MENU */
int recenv64_select(int mode);
int recenv64_confirm(void);              /* RECENV_MENU -> RECENV_RESOLVED */
int recenv64_countdown(int *seconds_left);
int recenv64_state(int *out);
int recenv64_selected_mode(int *out);

#endif