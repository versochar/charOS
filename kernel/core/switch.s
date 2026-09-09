.code32
.section .text
.global task_switch
.type task_switch, @function
/* void task_switch(uint32_t* old_esp, uint32_t* new_esp);
   old_esp: pointer to store current esp
   new_esp: pointer to load next esp
*/
task_switch:
    movl 4(%esp), %eax   /* old_esp */
    movl 8(%esp), %ecx   /* new_esp */

    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    movl %esp, (%eax)    /* save current esp */
    movl (%ecx), %esp    /* load next esp */

    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret
.size task_switch, .-task_switch

.global task_bootstrap
.type task_bootstrap, @function
/* İlk task girişi - interrupt'ları aç ve entry'yi çağır */
task_bootstrap:
    sti
    popl %eax            /* entry */
    call *%eax
    call task_exit
    /* task_exit dönerse halt */
.Lbootstrap_halt:
    hlt
    jmp .Lbootstrap_halt
.size task_bootstrap, .-task_bootstrap

.global jump_to_user_mode
.type jump_to_user_mode, @function
/* void jump_to_user_mode(uint32_t entry, uint32_t stack)
    Ring 3'e iret ile geçiş. Dönmez.
    entry = user EIP, stack = user ESP */
jump_to_user_mode:
    mov 4(%esp), %edx    /* entry (user eip) */
    mov 8(%esp), %ecx    /* stack (user esp) */

    mov $0x23, %ax       /* user data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov %ecx, %esp       /* user stack'a geç (iret frame burada) */
    pushl $0x23          /* user ss  */
    pushl %ecx           /* user esp */
    pushl $0x202         /* eflags: IF=1 */
    pushl $0x1B          /* user cs  */
    pushl %edx           /* user eip */
    iret
.size jump_to_user_mode, .-jump_to_user_mode

.global fork_enter_user
.type fork_enter_user, @function
/* 12B: Eski tekil - uyum için (artık kullanılmıyor) */
fork_enter_user:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    movl 12(%ebp), %ecx
    movl 16(%ebp), %edx
    movl 20(%ebp), %esi
    movl 24(%ebp), %edi
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    pushl %edi
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %eax
    xorl %eax, %eax
    iret
.size fork_enter_user, .-fork_enter_user

.global fork_enter_user_full
.type fork_enter_user_full, @function
/* 12B: Full restore - parent'ın general register'larını da kopyala.
 * void fork_enter_user_full(uint32_t eip, uint32_t esp, uint32_t eflags,
 *                           uint32_t cs, uint32_t ss,
 *                           uint32_t edi, uint32_t esi, uint32_t ebp,
 *                           uint32_t ebx, uint32_t edx, uint32_t ecx)
 * Dönmez (iret). eax=0 child */
fork_enter_user_full:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax    /* eip */
    movl 12(%ebp), %ecx   /* user esp */
    movl 16(%ebp), %edx   /* eflags */
    movl 20(%ebp), %esi   /* cs */
    movl 24(%ebp), %edi   /* ss */
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    /* iret frame: ss, esp, eflags, cs, eip */
    pushl %edi            /* ss */
    pushl %ecx            /* esp */
    pushl %edx            /* eflags */
    pushl %esi            /* cs */
    pushl %eax            /* eip */
    /* general regs */
    movl 28(%ebp), %edi
    movl 32(%ebp), %esi
    movl 40(%ebp), %ebx
    movl 44(%ebp), %edx
    movl 48(%ebp), %ecx
    movl 36(%ebp), %ebp   /* user ebp - en son (frame'i ezdikten sonra) */
    xorl %eax, %eax       /* fork() == 0 */
    iret
.size fork_enter_user_full, .-fork_enter_user_full

.global fork_jump_to_user
.type fork_jump_to_user, @function
/* 12B: simplified - only user_esp/eip/eflags needed, general regs 0 is fine for first fork test.
 * void fork_jump_to_user(struct task *t) */
fork_jump_to_user:
    mov 4(%esp), %eax
    push %eax
    push %ebx
    push %ecx
    push %edx
    mov 88(%eax), %ecx
    mov %ecx, %ebx
    mov %ebx, %eax
    call serial_puthex
    mov 4(%esp), %eax
    mov 92(%eax), %edx
    mov %edx, %ebx
    mov %ebx, %eax
    call serial_puthex
    mov 4(%esp), %eax
    pop %edx
    pop %ecx
    pop %ebx
    push %eax
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    pop %eax
    mov 88(%eax), %ecx
    mov 92(%eax), %edx
    mov %ecx, %esp
    pushl $0x23
    pushl %ecx
    pushl 96(%eax)
    pushl $0x1B
    pushl %edx
    xor %eax, %eax
    iret
.size fork_jump_to_user, .-fork_jump_to_user


