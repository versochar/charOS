#ifndef CHAROS_CORE_ISR_H
#define CHAROS_CORE_ISR_H

#include <stdint.h>

/* CPU tarafından otomatik push'lanan + bizim push'ladıklarımız */
struct registers {
    uint32_t ds;                /* Data segment - bizim push */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t int_no, err_code;  /* Bizim push'ladığımız int_no ve err_code */
    uint32_t eip, cs, eflags, useresp, ss; /* CPU tarafından push'lanan */
} __attribute__((packed));

typedef void (*isr_t)(struct registers*);

void isr_init(void);
void isr_handler(struct registers* regs);
void irq_handler(struct registers* regs);
void isr_register_handler(uint8_t n, isr_t handler);
void isr_unregister_handler(uint8_t n);

extern const char* exception_messages[32];

#endif /* CHAROS_CORE_ISR_H */