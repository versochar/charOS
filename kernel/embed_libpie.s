.section .rodata
.global libpie_start
.global libpie_end
libpie_start:
.incbin "build/libpie.so"
libpie_end: