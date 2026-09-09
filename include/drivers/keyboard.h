#ifndef CHAROS_DRIVERS_KEYBOARD_H
#define CHAROS_DRIVERS_KEYBOARD_H

#include <stdint.h>
#include <core/isr.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define KBD_CMD_PORT    0x64

void keyboard_init(void);
void keyboard_handler(struct registers* regs);
/* 26C: PS/2 baytını işle (USB-HID beslemesi için açık API) */
void keyboard_scancode(uint8_t scancode);
char keyboard_getchar(void);          // blocking
char keyboard_getchar_nonblock(void); // 0 if empty
int  keyboard_has_char(void);
void keyboard_flush(void);

/* Özel tuş kodları */
#define KEY_ENTER       '\n'
#define KEY_BACKSPACE   '\b'
#define KEY_ESC         27
#define KEY_TAB         '\t'

#endif