.section .rodata
.global libla_start
.global libla_end
libla_start:
.incbin "build/libla.so"
libla_end: