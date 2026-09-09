/* 19A: exec argv konvansiyonu — kernel taze stack'in tepesine
 * [argc][argv0]...[argvN-1][NULL][stringler] kurar, esp argc'yi gösterir.
 * _start naked olmalı (C prologu esp'yi bozar); umain(argc, argv) çağırır. */
typedef unsigned int u32;

#define SYS_EXIT_ARG 0

__attribute__((naked, section(".entry"))) void _start(void) {
    asm volatile(
        "movl %esp, %eax\n\t"
        "movl (%eax), %ebx\n\t"   /* argc */
        "leal 4(%eax), %ecx\n\t"  /* argv */
        "pushl %ecx\n\t"
        "pushl %ebx\n\t"
        "call umain\n\t"
        "addl $8, %esp\n\t"
        "movl %eax, %ebx\n\t"     /* çıkış kodu -> ebx */
        "xorl %ecx, %ecx\n\t"
        "xorl %edx, %edx\n\t"
        "movl $0, %eax\n\t"       /* SYS_EXIT */
        "int $0x80\n\t"
        "1: jmp 1b\n\t"
    );
    for (;;) { }
}
