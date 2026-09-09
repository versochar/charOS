.section .rodata
.global libtls_start
.global libtls_end
libtls_start:
.incbin "build/libtls.so"
libtls_end: