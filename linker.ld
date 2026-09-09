/* Linker script for charOS kernel */
ENTRY(_start)

SECTIONS
{
    /* Kernel 1MB'dan başlasın (GRUB multiboot standardı) */
    . = 1M;

    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot))
        *(.text)
        *(.rodata*)
        *(.eh_frame)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .bss ALIGN(4K) :
    {
        *(.bss)
        *(COMMON)
    }

    /* Kernel'in sonunu işaret et */
    kernel_end = .;
}