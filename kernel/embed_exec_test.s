.section .rodata
.global exec_test_start
.global exec_test_end
exec_test_start:
.incbin "build/exec_test.bin"
exec_test_end:
