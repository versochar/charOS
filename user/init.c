/* 14C: init (Ring3) - getty üretir, çıkışta yeniden başlatır.
 * Varsayılan açılışta çekirdek kabuğu çalışır; init, kernelsiz
 * userland önyükleme isteyen kurulumlar için /bin/init'tedir. */
typedef unsigned int uint32_t;
#define SYS_FORK 12
#define SYS_EXEC 15
#define SYS_WAITPID 19
#define SYS_EXIT 0
static inline uint32_t syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c){ uint32_t r; asm volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c):"memory"); return r; }
void puts(const char*s){ uint32_t l=0; while(s[l]) l++; syscall(1,(uint32_t)s,l,0); }
void init_main(void) {
    puts("charOS init: shell respawn loop\n");
    for (;;) {
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            /* çocuk: shell direkt */
            int ret = syscall(SYS_EXEC,(uint32_t)"/bin/sh",0,0);
            (void)ret;
            puts("init: shell exec failed\n");
            syscall(SYS_EXIT,1,0,0);
        } else {
            int st = 0;
            syscall(SYS_WAITPID,c,(uint32_t)&st,0);
            /* çıkış/kilitlenme -> döngü başı yeniden üretir */
        }
    }
}
__attribute__((section(".entry")))
void _start(void){ init_main(); }
