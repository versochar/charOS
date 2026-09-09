/* 26A: UEFI loader PE link script (i386pep).
 * Yalnızca gerekli bölümler; .comment/.note/.eh_frame/.debug hepsi atılır
 * (VMA=0 çakışması OVMF'yi "Unsupported"a düşürüyordu). */
OUTPUT_FORMAT("pei-x86-64", "pei-x86-64", "pei-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(efi_main)
SECTIONS
{
  .text 0x1000 : {
    *(.text .text.*)
    *(.rodata .rodata.*)
  }
  .data : {
    *(.data .data.*)
    *(.sdata .sdata.*)
    *(.bss .bss.*)
    *(COMMON)
  }
  /DISCARD/ : {
    *(.comment .comment.*)
    *(.note .note.*)
    *(.eh_frame .eh_frame.*)
    *(.debug* .gnu*)
  }
}
