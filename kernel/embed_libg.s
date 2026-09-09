.section .rodata
.global libg_start
.global libg_end
libg_start:
.incbin "build/libg.so"
libg_end: