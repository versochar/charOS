#ifndef CHAROS_NET_NET_H
#define CHAROS_NET_NET_H

#include <stdint.h>

/* Temel ağ yapıları (minimal IP/ARP) */
#define NET_MAC_SIZE 6
#define NET_IP_SIZE  4

struct mac_addr {
    uint8_t addr[NET_MAC_SIZE];
};

struct ip_addr {
    uint8_t addr[NET_IP_SIZE];
};

/* Paket türleri */
#define PKT_ARP_REQ 1
#define PKT_ARP_REPLY 2
#define PKT_IP 3

/* Ağ başlatma */
void net_init(void);
void net_send_packet(const uint8_t* data, int len);
void net_handle_packet(const uint8_t* data, int len);
void net_poll(void);

/* 21A: ICMP echo + UDP (real e1000 üstünde) */
void net_icmp_send(uint32_t dst_ip);
int net_icmp_hit(void);
void net_icmp_reset(void);
int udp_sock_open(uint16_t lport);
int udp_sock_send(int s, uint32_t dst_ip, uint16_t dport, const uint8_t* data, int len);
int net_selftest(void);

#endif
