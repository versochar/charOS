/* 14C: getty (Ring3) - seri konsolda login sorar, geçerli
 * kullanıcıda /bin/sh'e exec'ler. Başarılı girişi /tmp/login.ok
 * dosyasına yazar (son giriş kaydı + 14C testi kancası). */
typedef unsigned int uint32_t;
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_OPEN 10
#define SYS_CLOSE 11
#define SYS_EXEC 15
#define SYS_EXIT 0
#define O_CREATE 4
#define O_WRONLY 2
static inline uint32_t syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c){ uint32_t r; asm volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c):"memory"); return r; }
void puts(const char*s){ uint32_t l=0; while(s[l]) l++; syscall(SYS_WRITE,1,(uint32_t)s,l); }
int strcmp(const char*a,const char*b){ while(*a&&*a==*b){a++;b++;} return *a-*b; }
static int read_line(char* buf, int max) {
    int n = 0;
    while (n < max - 1) {
        char c; int r = syscall(SYS_READ,0,(uint32_t)&c,1);
        if (r <= 0) return -1; /* EOF/kilit */
        if (c == '\n' || c == '\r') break;
        if (c == '\b' || c == 127) { if (n > 0) n--; continue; }
        buf[n++] = c;
    }
    buf[n] = 0;
    return n;
}
static int valid_user(const char* name) {
    return strcmp(name,"root") == 0 || strcmp(name,"guest") == 0;
}
void getty_main(void) {
    char user[32];
    for (int tries = 0; tries < 3; tries++) {
        puts("charOS login: ");
        if (read_line(user, sizeof(user)) < 0) syscall(SYS_EXIT,1,0,0);
        puts("\n");
        if (user[0] && valid_user(user)) {
            /* Kayıt dosyası boot başına taze oluşur (ramfs); baştan yazılır */
            int fd = syscall(SYS_OPEN,(uint32_t)"/tmp/login.ok",O_CREATE|O_WRONLY,0);
            if (fd >= 0) {
                uint32_t l = 0; while (user[l]) l++;
                syscall(SYS_WRITE,fd,(uint32_t)user,l);
                syscall(SYS_CLOSE,fd,0,0);
            }
            int ret = syscall(SYS_EXEC,(uint32_t)"/bin/sh",0,0);
            (void)ret;
            puts("getty: exec sh failed\n");
            syscall(SYS_EXIT,1,0,0);
        }
        puts("login incorrect\n");
    }
    syscall(SYS_EXIT,1,0,0);
}
__attribute__((section(".entry")))
void _start(void){ getty_main(); }
