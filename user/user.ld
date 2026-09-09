OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x1000000;   /* 16MB - user code virtual address */

    .entry : {
        *(.entry)
    }

    .text : {
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.bss*)
    }
}
