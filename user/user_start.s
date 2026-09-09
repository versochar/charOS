.section .entry
.global _start
.type _start, @function
_start:
    jmp _main

.section .text
.global _main
.type _main, @function
/* User program main - user/userprog.c'den geliyor */
