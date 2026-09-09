.section .rodata
.global libj_start
.global libj_end
libj_start:
.incbin "build/libj.so"
libj_end: