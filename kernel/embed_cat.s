.section .rodata
.global cat_start
.global cat_end
cat_start:
.incbin "build/cat.bin"
cat_end:
