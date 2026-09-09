.section .multiboot
.align 4
.long 0x1BADB002              /* Multiboot magic number */
.long 0x00000007              /* Flags: align(1<<0), meminfo(1<<1), video(1<<2) */
.long -(0x1BADB002 + 0x00000007)  /* Checksum */
/* 18B: GRUB header struct'ı sabit offset'lidir; video alanları +32'de olur.
   Bit16 (aout) yokken bile 5 long (20B) padding ŞART, yoksa GRUB kodu okur. */
.long 0, 0, 0, 0, 0           /* header_addr..entry_addr padding (kullanılmaz) */
.long 0                       /* mode_type: 0 = linear framebuffer (+32) */
.long 1024                    /* width (+36) */
.long 768                     /* height (+40) */
.long 32                      /* depth (+44) */

/* 26A: UEFI boot için Multiboot2 başlığı (aynı .multiboot bölümünde KEEP'li).
   UEFI/GRUB (x64) 32-bit kernel yüklerken protected mode'a geçmek için
   multiboot2 protokolünü kullanır (multiboot v1 EFI üstünde long mode'da
   deadlock yapar -> #PF). GRUB BIOS'ta v1 başlığı da korunur. */
.align 8
mb2_start:
.long 0xE85250D6              /* multiboot2 magic */
.long 0                       /* arch: i386 protected mode */
.long mb2_end - mb2_start     /* header_length */
.long -(0xE85250D6 + 0 + (mb2_end - mb2_start))  /* checksum */

/* 26A notu: framebuffer header tag'i (type 5) GRUB 2.14'ün EFI
   multiboot2 yolunu kilitliyordu (video kurulumunda spin). Mod seçimi
   grub.cfg'deki gfxmode+gfxpayload=keep'e bırakıldı; GRUB framebuffer
   bilgisini boot info'ya yine de yazar. */
.align 8
.long 0                       /* type 0 = end */
.long 8                       /* size */
mb2_end:

.section .bss
.align 16
.global stack_bottom
.global stack_top
stack_bottom:
.skip 16384                   /* 16 KiB kernel stack */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    cli                       /* Kesmeleri kapat */
    /* 26A: en erken iz — COM1'e ham 'K' (UART OVMF/SeaBIOS'tan hazır).
       serial_init'ten önce çalışır; UEFI atlaması doğrulaması için.
       EAX (magic) korunur! */
    pushl %eax
    movw $0x3FD, %dx
    movl $1000000, %ecx
early_lsr:
    inb %dx, %al
    testb $0x20, %al
    jnz early_ok
    decl %ecx
    jnz early_lsr
early_ok:
    movw $0x3F8, %dx
    movb $'K', %al
    outb %al, %dx
    popl %eax
    movl $stack_top, %esp     /* Kernel stack'i ayarla */

    /* Multiboot bilgilerini sakla (GRUB eax=magic, ebx=info ptr) */
    pushl %eax                /* magic */
    pushl %ebx                /* mboot_ptr */

    /* GDT'yi yükle */
    lgdt gdt_ptr

    /* Protected mode'a geç (CR0.PE = 1) */
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    /* Far jump ile CS'yi kernel code segment'e (0x08) yükle */
    ljmp $0x08, $protected_mode

.code32
protected_mode:
    /* Data segmentleri kernel data'ya (0x10) ayarla */
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss

    /* Multiboot argümanlarını stack'ten al ve kernel_main'e çağır
       Dikkat: ESP zaten stack_top-8'de (push'lar korunuyor), yeniden ayarlama */
    popl %ebx                 /* mboot_ptr */
    popl %eax                 /* magic */
    pushl %ebx
    pushl %eax
    call kernel_main

    /* kernel_main dönerse sonsuz döngü */
    cli
halt_loop:
    hlt
    jmp halt_loop

.size _start, .-_start

/* ============================================================
 * GDT (Global Descriptor Table)
 * 0x00: Null descriptor
 * 0x08: Kernel Code (base=0, limit=4GB, exec/read, DPL=0)
 * 0x10: Kernel Data (base=0, limit=4GB, read/write, DPL=0)
 * 0x1B: User Code   (base=0, limit=4GB, exec/read, DPL=3)
 * 0x23: User Data   (base=0, limit=4GB, read/write, DPL=3)
 * 0x28: TSS         (dynamic, loaded via ltr)
 * ============================================================ */
.align 8
gdt_start:
gdt_null:
    .quad 0x0000000000000000

gdt_kernel_code:
    .quad 0x00CF9A000000FFFF  /* 0x08: P=1, DPL=0, S=1, Type=0xA (code, exec/read) */

gdt_kernel_data:
    .quad 0x00CF92000000FFFF  /* 0x10: P=1, DPL=0, S=1, Type=0x2 (data, read/write) */

gdt_user_code:
    .quad 0x00CFFB000000FFFF  /* 0x1B: P=1, DPL=3, S=1, Type=0xB (code, exec/read) */

gdt_user_data:
    .quad 0x00CFF3000000FFFF  /* 0x23: P=1, DPL=3, S=1, Type=0x3 (data, read/write) */

gdt_tss:
    .quad 0x0000890000000000  /* 0x28: Placeholder for TSS (P=1, DPL=0, Type=0x9) */

gdt_end:

gdt_ptr:
    .word gdt_end - gdt_start - 1  /* Limit (size - 1) */
    .long gdt_start                /* Base address */

/* gdt_flush ve tss_flush artık kernel/core/gdt.c içinde C olarak implemente edildi
   boot.s'deki eski stub'lar kaldırıldı (duplicate symbol önlemek için) */
