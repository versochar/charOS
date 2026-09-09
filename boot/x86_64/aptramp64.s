/* 33B: AP trampoline 64-bit (real mode -> long mode).
 *
 * Tamamen PIC: relocation YOK (call/pop ile runtime taban bulunur).
 * Her adreste calisir; yukleme onerisi 0x9000 (32-bit AP tramp 0x8000'de).
 * Loader yalnizca calisma-degerlerini yamalar: ap_slot_pml4 (PML4 phys <4G),
 * ap_slot_entry (64-bit AP girisi). Boyut <1KB (SIPI vector sayfasi).
 */
.code16
.global aptramp64_blob
.global aptramp64_end
.global ap_slot_pml4
.global ap_slot_entry

aptramp64_blob:
    .long 0x36505441          /* "APT6" magic (aptramp64_check) */
    cli
    /* DS=0 (real mode'da gecerli), tum erisimler ebx tabanli linear */
    xorw %ax, %ax
    movw %ax, %ds
    call getip
getip:
    pop %bx
    movw %cs, %ax
    movzwl %bx, %ebx
    shll $4, %eax
    addl %eax, %ebx
    subl $(getip - aptramp64_blob), %ebx  /* ebx = blob linear taban */
    /* PAE ac */
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4
    /* PML4 yukle (loader yamali, <4G) */
    movl (ap_slot_pml4 - aptramp64_blob)(%ebx), %eax
    movl %eax, %cr3
    /* LME ac */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr
    /* PG+PE ac */
    movl %cr0, %eax
    orl $0x80000001, %eax
    movl %eax, %cr0
    /* GDT desc tabanini yamala + yukle */
    leal (ap_gdt - aptramp64_blob)(%ebx), %eax
    movl %eax, (ap_gdtdesc + 2 - aptramp64_blob)(%ebx)
    lgdtl (ap_gdtdesc - aptramp64_blob)(%ebx)
    /* Uzak atlama vektorunu yamala: ap64 runtime + 0x08 */
    leal (ap64 - aptramp64_blob)(%ebx), %eax
    movl %eax, (ap_jmpvec - aptramp64_blob)(%ebx)
    movw $0x08, (ap_jmpvec + 4 - aptramp64_blob)(%ebx)
    ljmpl *(ap_jmpvec - aptramp64_blob)(%ebx)

.code64
ap64:
    xorl %eax, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %ss
    movq ap_slot_entry(%rip), %rax
    jmp *%rax                 /* 64-bit AP girisi (donus yok) */

.align 8
ap_slot_pml4:  .long 0        /* loader: PML4 phys dusuk 32 bit */
ap_slot_entry: .quad 0        /* loader: AP entry (RIP) */
ap_jmpvec:     .long 0        /* runtime: ap64 linear */
               .word 0        /* runtime: 0x08 */
ap_gdtdesc:    .word ap_gdt_end - ap_gdt - 1
               .long 0        /* runtime: ap_gdt linear */
.align 8
ap_gdt:
    .quad 0x0000000000000000
    .quad 0x00209A0000000000  /* 0x08: code64 L=1 */
    .quad 0x0000920000000000  /* 0x10: data64 */
ap_gdt_end:

aptramp64_end:
