/* 31A: 64-bit entry stub + GDT64
 * UEFI loader veya GRUB multiboot2 tarafindan 64-bit long mode'da
 * cagrilir. 32-bit kernel bozulmaz; bu stub ayri linklenir.
 * Girdi: rdi = multiboot2 info ptr (UEFI'den), rsp = gecerli stack
 */
.code64
.section .text64, "ax"
.global _start64
.type _start64, @function
_start64:
    cli
    /* segmentleri sifirla (long mode'da DS/ES/SS = 0) */
    xorl %eax, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %ss
    /* rdi (mbi ptr) korunur, kernel64_main'e tasinir */
    movq %rdi, %r12
    /* 31I: stack canary check icin stack hizalama */
    andq $-16, %rsp
    subq $8, %rsp
    movq %r12, %rdi
    call kernel64_main
    /* donerse dur */
halt64:
    hlt
    jmp halt64

/* 31G: GDT64 - null + code64 + data64 */
.section .rodata64, "a"
.align 16
.global gdt64_ptr
.global gdt64_table
gdt64_table:
    .quad 0x0000000000000000          /* 0x00 null */
    .quad 0x00209A0000000000          /* 0x08 code64: L=1, P=1, RX */
    .quad 0x0000920000000000          /* 0x10 data64: P=1, RW */
gdt64_ptr:
    .word gdt64_ptr - gdt64_table - 1
    .quad gdt64_table
