#include "arch/x86_64/longmode.h"
#include "arch/x86_64/tcp.h"

#define TCP_MAX 8
#define TCP_FREE 0
#define TCP_CREATED 1
#define TCP_BOUND 2
#define TCP_CONNECTED 3

typedef struct {
    u64 sock;
    int state;
    u64 pkt;
    int valid;
} tcp_sock_t;

static int tcp_inited = 0;
static u64 sock_ctr = 0;
static tcp_sock_t socks[TCP_MAX];

static tcp_sock_t *find_sock(u64 sock) {
    for (int i = 0; i < TCP_MAX; i++) {
        if (socks[i].state != TCP_FREE && socks[i].sock == sock) return &socks[i];
    }
    return 0;
}

int tcp64_init(void) {
    tcp_inited = 1;
    sock_ctr = 0;
    for (int i = 0; i < TCP_MAX; i++) {
        socks[i].sock = 0; socks[i].state = TCP_FREE; socks[i].valid = 0;
    }
    return 0;
}

int tcp64_socket(u64 *out_sock) {
    if (!tcp_inited || !out_sock) return -1;
    for (int i = 0; i < TCP_MAX; i++) {
        if (socks[i].state == TCP_FREE) {
            sock_ctr++;
            socks[i].sock = sock_ctr;
            socks[i].state = TCP_CREATED;
            socks[i].valid = 0;
            *out_sock = sock_ctr;
            return 0;
        }
    }
    return -1;
}

int tcp64_close(u64 sock) {
    tcp_sock_t *s = find_sock(sock);
    if (!s) return -1;
    s->state = TCP_FREE; s->sock = 0;
    return 0;
}

int tcp64_bind(u64 sock, u64 addr, u64 port) {
    (void)addr; (void)port;
    tcp_sock_t *s = find_sock(sock);
    if (!s) return -1;
    if (s->state != TCP_CREATED) return -2;
    s->state = TCP_BOUND;
    return 0;
}

int tcp64_connect(u64 sock, u64 addr, u64 port) {
    (void)addr; (void)port;
    tcp_sock_t *s = find_sock(sock);
    if (!s) return -1;
    if (s->state != TCP_BOUND) return -2;
    s->state = TCP_CONNECTED;
    return 0;
}

int tcp64_send(u64 sock, u64 val) {
    tcp_sock_t *s = find_sock(sock);
    if (!s) return -1;
    if (s->state != TCP_CONNECTED) return -2;
    if (s->valid) return -1;
    s->pkt = val;
    s->valid = 1;
    return 0;
}

int tcp64_recv(u64 sock, u64 *out_val) {
    tcp_sock_t *s = find_sock(sock);
    if (!s || !out_val) return -1;
    if (s->state != TCP_CONNECTED) return -2;
    if (!s->valid) return -1;
    *out_val = s->pkt;
    s->valid = 0;
    return 0;
}
