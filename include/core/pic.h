#ifndef CHAROS_CORE_PIC_H
#define CHAROS_CORE_PIC_H

#include <stdint.h>

/* PIC portları */
#define PIC1            0x20    /* Master PIC command */
#define PIC1_DATA       0x21    /* Master PIC data */
#define PIC2            0xA0    /* Slave PIC command */
#define PIC2_DATA       0xA1    /* Slave PIC data */
#define PIC_EOI         0x20    /* End Of Interrupt */

/* ICW1 bitleri */
#define ICW1_ICW4       0x01
#define ICW1_SINGLE     0x02
#define ICW1_INTERVAL4  0x04
#define ICW1_LEVEL      0x08
#define ICW1_INIT       0x10

/* ICW4 bitleri */
#define ICW4_8086       0x01
#define ICW4_AUTO       0x02
#define ICW4_BUF_SLAVE  0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM       0x10

void pic_init(void);
void pic_remap(uint8_t offset1, uint8_t offset2);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq_line);
void pic_clear_mask(uint8_t irq_line);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);
void pic_disable(void);

#endif /* CHAROS_CORE_PIC_H */