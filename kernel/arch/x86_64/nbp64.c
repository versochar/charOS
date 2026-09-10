#include "arch/x86_64/nbp.h"
#include <string.h>

static u32 nbp_ip = 0;          /* DHCP sunucu */
static u32 nbp_tftp = 0;        /* TFTP sunucu */
static char nbp_bootfile[NBP64_PATH_MAX];
static int nbp_state = NBP64_IDLE;
static int nbp_attempt = 0;
static int nbp_ready = 0;       /* bootfile secildi mi */

int nbp64_init(void) {
    nbp_ip = 0;
    nbp_tftp = 0;
    memset(nbp_bootfile, 0, sizeof(nbp_bootfile));
    nbp_state = NBP64_IDLE;
    nbp_attempt = 0;
    nbp_ready = 0;
    return 0;
}

int nbp64_set_server(u32 ip, u32 tftp_ip) {
    if (!ip || !tftp_ip) return -1;
    nbp_ip = ip;
    nbp_tftp = tftp_ip;
    return 0;
}

int nbp64_set_bootfile(const char *path) {
    if (!path) return -1;
    if (strlen(path) >= NBP64_PATH_MAX) return -2;
    strncpy(nbp_bootfile, path, sizeof(nbp_bootfile) - 1);
    nbp_ready = 1;
    return 0;
}

int nbp64_discover(void) {
    if (!nbp_ip || !nbp_tftp) return -1; /* sunucu yok */
    if (nbp_state != NBP64_IDLE) return -2;
    nbp_attempt = 0;
    nbp_state = NBP64_DHCP;
    return 0;
}

int nbp64_poll(void) {
    if (nbp_state == NBP64_IDLE || nbp_state == NBP64_EXEC)
        return -1; /* bekleme yok */
    if (!nbp_ready) {
        nbp_attempt++;
        if (nbp_attempt >= NBP64_MAX_ATTEMPTS) {
            nbp_state = NBP64_FAILED;
            return -2; /* yeniden deneme tukendi */
        }
        nbp_state = NBP64_DHCP;
        return 1; /* tekrar dene */
    }
    if (nbp_state == NBP64_DHCP) {
        nbp_state = NBP64_TFTP_DOWNLOAD;
        return 0;
    }
    return -1;
}

int nbp64_boot(void) {
    if (!nbp_ready) return -1;
    if (nbp_state != NBP64_TFTP_DOWNLOAD) return -2;
    nbp_state = NBP64_EXEC;
    return 0;
}

int nbp64_reset(void) {
    nbp_attempt = 0;
    nbp_state = NBP64_IDLE;
    return 0;
}

int nbp64_state(int *out) {
    if (!out) return -1;
    *out = nbp_state;
    return 0;
}

int nbp64_attempts(void) {
    return nbp_attempt;
}