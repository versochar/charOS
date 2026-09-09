.section .rodata
.global libmath_start
.global libmath_end
libmath_start:
.incbin "build/libmath.so"
libmath_end:
