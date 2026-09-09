/* 26A: UEFI loader link script (base 0, -shared; .reloc OVMF taşır) */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(efi_main)
SECTIONS
{
    . = 0;
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }
    .data : {
        *(.data .data.*)
        *(.sdata .sdata.*)
    }
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    /DISCARD/ : {
        *(.eh_frame .eh_frame.*)
        *(.comment .comment.*)
        *(.note .note.*)
    }
}
