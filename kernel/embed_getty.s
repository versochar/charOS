.section .rodata
.global getty_start
.global getty_end
getty_start:
.incbin "build/getty.bin"
getty_end:
