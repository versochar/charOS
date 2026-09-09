.section .rodata
.global libb_start
.global libb_end
libb_start:
.incbin "build/libb.so"
libb_end: