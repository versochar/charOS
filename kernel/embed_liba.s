.section .rodata
.global liba_start
.global liba_end
liba_start:
.incbin "build/liba.so"
liba_end: