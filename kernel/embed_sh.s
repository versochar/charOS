.section .rodata
.global sh_start
.global sh_end
sh_start:
.incbin "build/sh.bin"
sh_end:
