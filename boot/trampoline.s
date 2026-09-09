/* 11D: AP Trampoline - SIPI vektör 8 = 0x8000 fiziksel adresine kopyalanır */
/* Real mode'da başlar -> protected mode -> paging açar -> C ap_main'e atlar.
   Kernel sembollerini referans edemez (0x8000'de ayrık linkli).
   İhtiyaç duyduğu her şeyi 0x7000 adresindeki sabit SMP bilgi bloğundan okur.
   Bilgi bloğu dizilimi (kernel/core/apic.c'deki s*mp_info_block ile eşleşir):
     +0x00  u16 gdt_limit
     +0x02  u32 gdt_base
     +0x06  u16 idt_limit
     +0x08  u32 idt_base
     +0x0C  u32 cr3
     +0x10  u32 ap_stack
     +0x14  u32 ap_entry
*/

/* Sabit düşük bellek adresleri */
.equ SMP_INFO,       0x7000
.equ INFO_GDT,       SMP_INFO + 0x00
.equ INFO_IDT,       SMP_INFO + 0x06
.equ INFO_CR3,       SMP_INFO + 0x0C
.equ INFO_STACK,     SMP_INFO + 0x10
.equ INFO_ENTRY,     SMP_INFO + 0x14

.section .text
.code16
.global ap_tramp_start
.global ap_tramp_end

ap_tramp_start:
    cli
    cld

    /* Real mode veri segmentleri: DS=0 (0x7000 erişimi için) */
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss

    /* Kernel GDT'sini yükle (limit+base 0x7000'de, 6 byte) */
    lgdtl INFO_GDT

    /* Protected mode aç (PE) */
    movl %cr0, %eax
    orl  $1, %eax
    movl %eax, %cr0

    /* Far jump: CS=0x08 (kernel code), 32-bit offset (link VMA 0x8000) */
    ljmpl $0x08, $ap_prot32

.code32
ap_prot32:
    /* Veri segmentleri = kernel data 0x10 */
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss

    /* Paging: CR3 = kernel page_directory (identity, 0x7000 hâlâ okunur) */
    movl INFO_CR3, %eax
    movl %eax, %cr3
    movl %cr0, %eax
    orl  $0x80000000, %eax
    movl %eax, %cr0

    /* Kernel IDT'sini yükle (0x7006'da limit+base, 6 byte) */
    lidtl INFO_IDT

    /* AP stack'ini bilgi bloğundan al */
    movl INFO_STACK, %esp

    /* C girişine atla: ap_main(apic.c) */
    movl INFO_ENTRY, %eax
    jmp  *%eax

ap_tramp_end:

.section .note.GNU-stack,"",@progbits
