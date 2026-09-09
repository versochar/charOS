.section .rodata
.global libg2_start
.global libg2_end
libg2_start:
.incbin "build/libg2.so"
libg2_end: