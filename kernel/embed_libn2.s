.section .rodata
.global libn2_start
.global libn2_end
libn2_start:
.incbin "build/libn2.so"
libn2_end: