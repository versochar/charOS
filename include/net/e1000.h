#ifndef CHAROS_NET_E1000_H
#define CHAROS_NET_E1000_H

#include <stdint.h>

int e1000_init(void);
int e1000_send(const uint8_t* data, int len);
void e1000_receive(void);
int e1000_present(void);
void e1000_mac_get(uint8_t* out);
int e1000_link(void);
void e1000_poll(void);
void e1000_timer_poll(void);

#endif
