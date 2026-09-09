.section .rodata
.global init_start
.global init_end
init_start:
.incbin "build/init.bin"
init_end:
