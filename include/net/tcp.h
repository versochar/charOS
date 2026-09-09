#ifndef CHAROS_NET_TCP_H
#define CHAROS_NET_TCP_H

#include <stdint.h>

/* 10A: Temel TCP sunucu/istemci (minimal HTTP) */

#define TCP_PORT_HTTP 80

void tcp_server_start(void);
void tcp_client_connect(const char* ip_str, int port);
void tcp_send(const char* data, int len);
int tcp_recv(char* buf, int len);

/* 14I: TCP v2 — durum makinesi + yeniden iletim + pencere.
 * Döngüsel test (loopback enjeksiyon) ile doğrulanır; tel sürücüsü
 * hazır olunca aynı çekirdek gerçek hatta çıkar. */

#define TCP_STATE_CLOSED 0
#define TCP_STATE_SYN_SENT 1
#define TCP_STATE_ESTABLISHED 2
#define TCP_STATE_CLOSE_WAIT 3

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

#define TCP_MSS 512

int tcp_init(void);
int tcp_connect(uint32_t remote_ip, uint16_t remote_port); /* pcb id / -1 */
int tcp_send_data(int id, const char* data, int len);      /* yazılan bayt */
int tcp_state(int id);
uint32_t tcp_snd_nxt(int id);
uint32_t tcp_snd_una(int id);
uint32_t tcp_rcv_nxt(int id);
int tcp_rlen(int id);              /* alınan tampon baytı */
int tcp_rbuf_get(int id, char* out, int max); /* tüketmeden kopyala */
void tcp_tick(void);               /* RTO süresi dolanları yeniden gönder */
void tcp_set_rto(uint32_t ticks);
void tcp_set_rwnd(int id, uint16_t wnd);
/* Test kancaları: eşten gelen segmenti kur/gönder */
int tcp_inject(int id, uint8_t flags, uint32_t seq, uint32_t ack,
               const char* data, int len);
int tcp_craft(uint8_t* out, int max, uint8_t flags, uint32_t seq,
              uint32_t ack, const char* data, int len);
int tcp_inject_raw(const uint8_t* frame, int len);
int tcp_input(const uint8_t* frame, int len); /* ham çerçeve */
/* Giden günlüğü (yeniden iletim kanıtı) */
int tcp_tx_count(void);
int tcp_tx_get(int i, uint32_t* seq, uint32_t* ack, uint8_t* flags, int* len);
int tcp_tx_data(int i, char* out, int max);
int tcp_selftest(void);            /* el sıkışma/rto/pencere/cksum. 0 PASS. */

#endif
