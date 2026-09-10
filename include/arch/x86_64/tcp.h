#ifndef TCP_H
#define TCP_H

#include "arch/x86_64/longmode.h"

int tcp64_init(void);
int tcp64_socket(u64 *out_sock);
int tcp64_close(u64 sock);
int tcp64_bind(u64 sock, u64 addr, u64 port);
int tcp64_connect(u64 sock, u64 addr, u64 port);
int tcp64_send(u64 sock, u64 val);
int tcp64_recv(u64 sock, u64 *out_val);

#endif
