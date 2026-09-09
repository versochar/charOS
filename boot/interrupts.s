.code32
.section .text

/* Makrolar */
.macro ISR_NOERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $0          /* dummy err_code */
    pushl $\num       /* int_no */
    jmp isr_common_stub
.endm

.macro ISR_ERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushl $\num       /* int_no - err_code zaten CPU tarafından push'landı */
    jmp isr_common_stub
.endm

.macro IRQ num, map
.global irq\num
.type irq\num, @function
irq\num:
    cli
    pushl $0
    pushl $\map
    jmp irq_common_stub
.endm

/* 0-31 ISR'lar */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

/* 32-47 IRQ'lar (PIC) */
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

/* 11E: LAPIC timer (vektör 0x40) - isr_handler'a yönlendir, EOI handler'da */
.global isr40
.type isr40, @function
isr40:
    cli
    pushl $0          /* dummy err_code */
    pushl $0x40       /* int_no = 0x40 */
    jmp isr_common_stub

/* 11C: TLB shootdown IPI (LAPIC vektör 0x50) - isr_handler'a yönlendir */
.global isr50
.type isr50, @function
isr50:
    cli
    pushl $0          /* dummy err_code */
    pushl $0x50       /* int_no = 0x50 */
    jmp isr_common_stub

/* Genel spurious/boş kesme stub'ı (hemen iret) */
.global isr_spurious
.type isr_spurious, @function
isr_spurious:
    iret

/* APIC spurious vektörü (0xFF): Intel kuralı gereği EOI GÖNDERİLMEZ, sadece iret */
.global isr_spurious_apic
.type isr_spurious_apic, @function
isr_spurious_apic:
    iret

/* Ortak ISR stub - C isr_handler'ı çağırır */
.extern isr_handler
.type isr_common_stub, @function
isr_common_stub:
    pusha                   /* eax,ecx,edx,ebx,esp,ebp,esi,edi */
    movw %ds, %ax
    pushl %eax
    movw $0x10, %ax          /* kernel data selector */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp              /* registers* arg */
    call isr_handler
    addl $4, %esp

    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    popa
    addl $8, %esp            /* int_no ve err_code temizle */
    sti
    iret

/* Ortak IRQ stub - C irq_handler'ı çağırır */
.extern irq_handler
.type irq_common_stub, @function
irq_common_stub:
    pusha
    movw %ds, %ax
    pushl %eax
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp
    call irq_handler
    addl $4, %esp

    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    popa
    addl $8, %esp
    sti
    iret

/* int 0x80 syscall stub - user mode'dan çağrılır */
.extern syscall_handler
.global syscall_stub
.type syscall_stub, @function
syscall_stub:
    cli
    pushl $0          /* dummy err_code */
    pushl $0x80       /* int_no = 0x80 */
    pusha
    movw %ds, %ax
    pushl %eax
    movw $0x10, %ax   /* kernel data selector */
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    pushl %esp        /* registers* arg */
    call syscall_handler
    addl $4, %esp
    popl %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    popa
    addl $8, %esp     /* int_no + err_code temizle */
    iret
