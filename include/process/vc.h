#ifndef CHAROS_PROCESS_VC_H
#define CHAROS_PROCESS_VC_H

#include <stdint.h>
#include <process/pipe.h>

/* 18F: Virtual Channel (vc) - sunucu <-> istemci IPC kanalı.
 * Tek struct pipe her yönde (tam çift yönlü yerine tek yönlü mesaj kanalları
 * yeterli; window server tekil tx'i cmd->client reply için senkron kullanır). */

struct vc {
    struct pipe* up;    /* client -> server */
    struct pipe* down;  /* server -> client */
};

int vc_create_pair(struct task* server_task, struct task* client_task);
void vc_destroy(int upfd, int downfd);

/* Blocking mesaj yaz/oku (byte tabanlı, mevcut pipe ortogonal) */
int vc_write(int fd, const void* buf, int len);
int vc_read(int fd, void* buf, int len);

#endif
