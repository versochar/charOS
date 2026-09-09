.section .rodata
.global libg3_start
.global libg3_end
libg3_start:
.incbin "build/libg3.so"
libg3_end: