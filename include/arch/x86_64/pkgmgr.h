#ifndef PKGMGR64_H
#define PKGMGR64_H
#include "arch/x86_64/longmode.h"

#define PKGMGR64_MAX_PKGS 64
#define PKGMGR64_NAME_MAX  48

enum pkgmgr64_state {
    PKGMGR_IDLE = 0,
    PKGMGR_OP_IN_PROGRESS = 1,
    PKGMGR_FULL = 2,
};

int pkgmgr64_init(void);
int pkgmgr64_install(const char *name, const char *version);
int pkgmgr64_remove(const char *name);
int pkgmgr64_query(const char *name, char *version, int max);
int pkgmgr64_upgrade_all(void);
int pkgmgr64_list_names(char *out[], int max);
int pkgmgr64_installed_count(void);
int pkgmgr64_state(int *out);

#endif