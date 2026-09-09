.section .rodata
.global libk_start
.global libk_end
libk_start:
.incbin "build/libk.so"
libk_end: