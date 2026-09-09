#ifndef CHAROS_DRIVERS_SERIAL_H
#define CHAROS_DRIVERS_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8

void serial_init(void);
int serial_is_ready(void);
void serial_putc(char c);
void serial_puts(const char* str);
void serial_puthex(uint32_t val);
void serial_printf(const char* fmt, ...);
int serial_is_transmit_empty(void);

#endif /* CHAROS_DRIVERS_SERIAL_H */