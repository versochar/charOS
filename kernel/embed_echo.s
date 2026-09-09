.section .rodata
.global echo_start
.global echo_end
echo_start:
.incbin "build/echo.bin"
echo_end:
