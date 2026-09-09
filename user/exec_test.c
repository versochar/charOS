typedef unsigned int uint32_t;
#define SYS_WRITE 1
#define SYS_EXIT 0
static inline uint32_t syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c) {
    uint32_t ret; asm volatile("int $0x80" : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory"); return ret;
}
void up_puts(const char* s){ uint32_t len=0; while(s[len]) len++; syscall(SYS_WRITE,1,(uint32_t)s,len); }
__attribute__((section(".entry")))
void _start(void){
    up_puts("12C exec_test: OK\n");
    syscall(SYS_EXIT,0,0,0);
    for(;;) asm volatile("hlt");
}
