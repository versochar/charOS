/* 43D: UDP soket — tablo + geri-dongu halka tamponu (skeleton tasiyici).
 * Gercek NIC gonderimi 44x suruculerde; API simdiden sabit.
 */
#include "arch/x86_64/longmode.h"

#define UDP64_MAX 16
#define UDP64_BUF 2048

struct udp64_sock {
    int used;
    u32 local_ip;
    u16 local_port;
    u32 remote_ip;
    u16 remote_port;
    unsigned char rx[UDP64_BUF];
    u64 rx_head;
    u64 rx_len;
};

static struct udp64_sock udp64_tab[UDP64_MAX];

int udp64_socket(void) {
    int i;
    for (i = 0; i < UDP64_MAX; i++) {
        if (!udp64_tab[i].used) {
            udp64_tab[i].used = 1;
            udp64_tab[i].local_ip = 0;
            udp64_tab[i].local_port = 0;
            udp64_tab[i].remote_ip = 0;
            udp64_tab[i].remote_port = 0;
            udp64_tab[i].rx_head = 0;
            udp64_tab[i].rx_len = 0;
            return i;
        }
    }
    return -1;
}

static struct udp64_sock *udp64_get(int id) {
    if (id < 0 || id >= UDP64_MAX || !udp64_tab[id].used) return 0;
    return &udp64_tab[id];
}

int udp64_bind(int id, u32 ip, u16 port) {
    struct udp64_sock *s = udp64_get(id);
    int i;
    if (!s || !port) return -1;
    for (i = 0; i < UDP64_MAX; i++) {
        if (i != id && udp64_tab[i].used &&
            udp64_tab[i].local_port == port)
            return -2; /* port dolu */
    }
    s->local_ip = ip;
    s->local_port = port;
    return 0;
}

int udp64_connect(int id, u32 ip, u16 port) {
    struct udp64_sock *s = udp64_get(id);
    if (!s || !port) return -1;
    s->remote_ip = ip;
    s->remote_port = port;
    return 0;
}

int udp64_send(int id, const void *buf, u64 len) {
    struct udp64_sock *s = udp64_get(id);
    const unsigned char *p;
    u64 i;
    if (!s || !buf) return -1;
    if (!len || len > UDP64_BUF - s->rx_len) return -2;
    /* Skeleton: geri-dongu — ayni soketin rx kuyruguna */
    p = (const unsigned char *)buf;
    for (i = 0; i < len; i++)
        s->rx[(s->rx_head + s->rx_len + i) % UDP64_BUF] = p[i];
    s->rx_len += len;
    return (int)len;
}

int udp64_recv(int id, void *buf, u64 max) {
    struct udp64_sock *s = udp64_get(id);
    unsigned char *d;
    u64 n, i;
    if (!s || !buf) return -1;
    if (!s->rx_len) return 0; /* bloklamayan: veri yok */
    n = s->rx_len < max ? s->rx_len : max;
    d = (unsigned char *)buf;
    for (i = 0; i < n; i++)
        d[i] = s->rx[(s->rx_head + i) % UDP64_BUF];
    s->rx_head = (s->rx_head + n) % UDP64_BUF;
    s->rx_len -= n;
    return (int)n;
}

int udp64_close(int id) {
    struct udp64_sock *s = udp64_get(id);
    if (!s) return -1;
    s->used = 0;
    return 0;
}
