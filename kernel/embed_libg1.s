.section .rodata
.global libg1_start
.global libg1_end
libg1_start:
.incbin "build/libg1.so"
libg1_end: