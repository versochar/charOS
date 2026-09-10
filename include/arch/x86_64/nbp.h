#ifndef NBP64_H
#define NBP64_H
#include "arch/x86_64/longmode.h"

#define NBP64_MAX_ATTEMPTS 5
#define NBP64_PATH_MAX     64

enum nbp64_state {
    NBP64_IDLE = 0,
    NBP64_DHCP   = 1,
    NBP64_TFTP_DOWNLOAD = 2,
    NBP64_EXEC = 3,
    NBP64_FAILED = 4,
};

int nbp64_init(void);
int nbp64_set_server(u32 ip, u32 tftp_ip);
int nbp64_set_bootfile(const char *path);
int nbp64_discover(void);
int nbp64_poll(void);
int nbp64_boot(void);
int nbp64_reset(void);
int nbp64_state(int *out);
int nbp64_attempts(void);

#endif