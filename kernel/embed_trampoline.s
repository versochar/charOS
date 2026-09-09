.section .rodata
.global trampoline_blob_start
.global trampoline_blob_end
trampoline_blob_start:
.incbin "build/trampoline.bin"
trampoline_blob_end:
