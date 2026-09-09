/* 26A: long mode -> 32-bit protected mode trampoline.
 *
 * Loader bu blob'u <4G bir tampona kopyalar ve oradan çağırır
 * (ms ABI: RCX=kentry, RDX=mbi). Blob tamamen pozisyon-bağımsızdır:
 * 64-bit kısım RIP-relative, 32-bit kısım call/pop ile taban bulur.
 */
.code64
.global uefi_trampoline_blob
.global uefi_trampoline_end

uefi_trampoline_blob:
    /* RCX = kernel entry (<4G), RDX = mbi tabanı (<4G) */
    movl $0xA8, 0x500             /* iz: blob girildi */
    movb $0xA8, %al
    outb %al, $0x80               /* POST aynası: boot services kapalı, port yaşar */
    /* A8: ekran kırmızı (long mode, firmware mapping aktif, FB phys erişilir) */
    movq slot_fb(%rip), %rsi
    testq %rsi, %rsi
    jz fill8_skip
    movl slot_fbp(%rip), %r10d
    movl slot_fbw(%rip), %r11d
    movl slot_fbh(%rip), %r8d
    testl %r10d, %r10d
    jz fill8_skip
    testl %r11d, %r11d
    jz fill8_skip
    testl %r8d, %r8d
    jz fill8_skip
    movl $0x00FF0000, %r9d
fill8_row:
    movl %r11d, %eax
    movq %rsi, %rdi
fill8_col:
    movl %r9d, (%rdi)
    addq $4, %rdi
    decl %eax
    jnz fill8_col
    addq %r10, %rsi
    decl %r8d
    jnz fill8_row
fill8_skip:
    movq %rcx, slot_kentry(%rip)
    movl %edx, slot_mbi(%rip)
    cli
    /* GDT'yi blob içine kur */
    leaq gdt(%rip), %rax
    movq %rax, gdtdesc_base(%rip)
    lgdt gdtdesc(%rip)
    /* compat (32-bit) segmente geç */
    pushq $0x08
    leaq compat(%rip), %rax
    pushq %rax
    lretq

.align 8
slot_kentry:  .quad 0
slot_mbi:     .long 0
/* GOP sahne doldurma slotları (loader yamalar; A8/A9/AA renkleri) */
.global slot_fb
.global slot_fbp
.global slot_fbw
.global slot_fbh
slot_fb:      .quad 0   /* framebuffer phys base */
slot_fbp:     .long 0   /* pitch (bayt) */
slot_fbw:     .long 0   /* genislik (px) */
slot_fbh:     .long 0   /* yukseklik (px) */
gdtdesc:      .word (gdt_end - gdt - 1)
gdtdesc_base: .quad 0
.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00cf9a000000ffff  /* 0x08: 32-bit code, D=1 L=0 */
    .quad 0x00cf92000000ffff  /* 0x10: 32-bit data */
gdt_end:

.code32
compat:
    movl $0xA9, 0x500             /* iz: compat mod */
    movb $0xA9, %al
    outb %al, $0x80
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    /* EDX hâlâ mbi: 32-bit yığını mbi tamponunun tepesi yap (RSP 64-bit
       olabilir, call öncesi ESP şart) */
    leal 8192(%edx), %esp
    call getbase
getbase:
    popl %esi                 /* esi = getbase çalışma adresi */
    subl $OFF_GETBASE, %esi   /* esi = blob tabanı */
    /* A9: ekran mavi (compat: paging HÂLÂ açık, firmware mapping aktif;
     * FB >4GB ise 32-bitten erişilemez -> atla) */
    movl OFF_FB+4(%esi), %edi
    testl %edi, %edi
    jnz fill9_skip
    movl OFF_FB(%esi), %edi
    testl %edi, %edi
    jz fill9_skip
    movl OFF_FBP(%esi), %ebx
    movl OFF_FBW(%esi), %edx
    movl OFF_FBH(%esi), %ecx
    testl %ebx, %ebx
    jz fill9_skip
    testl %edx, %edx
    jz fill9_skip
    testl %ecx, %ecx
    jnz fill9_go
    jmp fill9_skip
fill9_go:
    movl $0x000000FF, %eax
    movl %esi, %ebp
    movl %edx, %esi
    shll $2, %esi
    subl %esi, %ebx           /* ebx = pitch - satir bayti */
fill9_row:
    movl %edx, %esi
fill9_col:
    movl %eax, (%edi)
    addl $4, %edi
    decl %esi
    jnz fill9_col
    addl %ebx, %edi
    decl %ecx
    jnz fill9_row
    movl %ebp, %esi
fill9_skip:
    /* PG kapat (PE korunur), WP kapat (BIOS paritesi: kernel supervisor
     * RO yazımlara dayanır — CoW fork finalize. UEFI WP=1 bırakır!) */
    movl %cr0, %eax
    andl $0x7FFEFFFF, %eax
    movl %eax, %cr0
    /* LME kapat */
    movl $0xC0000080, %ecx
    rdmsr
    andl $0xfffffeff, %eax
    wrmsr
    /* PAE kapat (BIOS/GRUBvarsayılanı) */
    movl %cr4, %eax
    andl $0xffffffdf, %eax
    movl %eax, %cr4
    /* prot'a uzak atla (çalışma-adresi + offset) */
    movl $OFF_PROT, %eax
    addl %esi, %eax
    pushl $0x08
    pushl %eax
    lret
prot:
    movl $0xAA, 0x500             /* iz: 32-bit protected, PG/LME/PAE kapalı */
    movb $0xAA, %al
    outb %al, $0x80
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    /* AA: ekran beyaz (prot: paging KAPALI, direkt phys; FB >4GB ise atla).
     * Beyaz ekran = trampoline bitti, kernel _start'a atlanıyor. */
    movl OFF_FB+4(%esi), %edi
    testl %edi, %edi
    jnz filla_skip
    movl OFF_FB(%esi), %edi
    testl %edi, %edi
    jz filla_skip
    movl OFF_FBP(%esi), %ebx
    movl OFF_FBW(%esi), %edx
    movl OFF_FBH(%esi), %ecx
    testl %ebx, %ebx
    jz filla_skip
    testl %edx, %edx
    jz filla_skip
    testl %ecx, %ecx
    jnz filla_go
    jmp filla_skip
filla_go:
    movl $0x00FFFFFF, %eax
    movl %esi, %ebp
    movl %edx, %esi
    shll $2, %esi
    subl %esi, %ebx
filla_row:
    movl %edx, %esi
filla_col:
    movl %eax, (%edi)
    addl $4, %edi
    decl %esi
    jnz filla_col
    addl %ebx, %edi
    decl %ecx
    jnz filla_row
    movl %ebp, %esi
filla_skip:
    movl OFF_MBI(%esi), %ebx
    movl $0x7C00, %esp
    movl $0x36D76289, %eax    /* mb2 magic */
    movl OFF_KENTRY(%esi), %ecx
    jmp *%ecx                 /* kernel _start (dönüş yok) */

/* blob-içi offset sabitleri (32-bit PIC için) */
.set OFF_GETBASE, getbase - uefi_trampoline_blob
.set OFF_PROT,    prot - uefi_trampoline_blob
.set OFF_MBI,     slot_mbi - uefi_trampoline_blob
.set OFF_KENTRY,  slot_kentry - uefi_trampoline_blob
.set OFF_FB,      slot_fb - uefi_trampoline_blob
.set OFF_FBP,     slot_fbp - uefi_trampoline_blob
.set OFF_FBW,     slot_fbw - uefi_trampoline_blob
.set OFF_FBH,     slot_fbh - uefi_trampoline_blob

uefi_trampoline_end:
