.section .rodata
.global libe_start
.global libe_end
libe_start:
.incbin "build/libe.so"
libe_end: