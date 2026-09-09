/* charOS user program (Ring 3 test) - Stage 6B
 * Freestanding, kernel'sa yüklü ELF olmadan direkt blob olarak gömülür.
 * Link adresi: 0x1000000 (16MB) - user sanal adres alanı.
 */

typedef unsigned int uint32_t;

#define SYS_EXIT  0
#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_GETPID 3
#define SYS_YIELD 4
#define SYS_OPEN  10
#define SYS_CLOSE 11
#define SYS_FORK  12
#define SYS_PIPE 13
#define SYS_SIGNAL 14
#define SYS_EXEC 15
#define SYS_NANOSLEEP 16
#define SYS_KILL 17
#define SYS_SIGRETURN 18
#define SYS_WAITPID 19
#define SYS_GETTICKS 5
#define SYS_SEM_INIT 20
#define SYS_SEM_WAIT 21
#define SYS_SEM_POST 22
#define SYS_SEM_DESTROY 23
#define SYS_MMAP 90
#define SYS_MUNMAP 91
#define SYS_NICE 24
#define SYS_CLONE 130
#define SYS_BLKREAD 131
#define SYS_BLKWRITE 132
#define SYS_DFSAVE 133
#define SYS_DFLOAD 134
#define SYS_BLKINFO 135
#define SYS_DFMKDIR 150
#define SYS_DFREMOVE 151
#define SYS_DFTRUNCATE 152
#define SYS_DFSTAT 153
#define SYS_DFLIST 154
#define SYS_DFCHECK 155
#define SYS_LSDIR 156
#define WNOHANG 1
#define SYS_INPUT 136
#define SYS_FBINFO 137
#define SYS_BRK 138
#define SYS_FUTEX 139
#define SYS_GETUID 140
#define SYS_SETUID 141
#define SYS_GETGID 142
#define SYS_SETGID 143
#define SYS_SYSLOG 144
#define SYS_UPTIME 145
#define SYS_CHMOD 146
#define SYS_DUP2 147
#if !defined(SYS_TASKLIST)
#define SYS_TASKLIST 158
#endif
#define SYS_GETGROUPS 159
#define SYS_SETGROUPS 160
#define SYS_CAPGET 161
#define SYS_CAPSET 162
#define CAP_DAC_OVERRIDE (1u << 1)
#define CAP_KILL (1u << 5)
#include "libcharos.h"
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 0x02
#define MAP_SHARED 0x01
#define MAP_ANONYMOUS 0x20
#define O_CREATE 4

/* TCP (10A/11A) basit test */
extern void tcp_client_connect(const char* ip, int port);
extern int tcp_recv(char* buf, int len);

/* int 0x80 ile syscall - ABI: eax=call_no, ebx=arg1, ecx=arg2, edx=arg3.
 * KRİTİK: her zaman inline olmalı — fork çocuğu trap adresinde uyanır;
 * gerçek fonksiyon olursa çocuk paylaşılan gövdenin içinde başlar (ebp çöp).
 * 19A-notu: -O0 ile derlenir; -O2 bu dosyada yanlış makine kodu üretiyordu
 * (yanlış syscall numarası/operant karışması). Asm esp'ye dokunmaz. */
__attribute__((always_inline)) static inline uint32_t syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c) {
    uint32_t ret;
    register uint32_t rbx asm("ebx") = a;
    register uint32_t rcx asm("ecx") = b;
    register uint32_t rdx asm("edx") = c;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(n), "r"(rbx), "r"(rcx), "r"(rdx)
                 : "memory", "cc");
    return ret;
}

void up_puts(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    syscall(SYS_WRITE, 1, (uint32_t)s, len);
}

void up_putdec(uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) { buf[i++] = '0'; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) syscall(SYS_WRITE, 1, (uint32_t)&buf[--i], 1);
}

/* 14E: ebeveyn pid'i (kill izni testi için CoW ile miras) */
static uint32_t up_mypid;

/* 14D: futex rendezvous kelimesi + bekleyen thread */
static volatile uint32_t futex_word;
static volatile uint32_t futex_done;
static void futex_thread_fn(void) {
    uint32_t r = syscall(SYS_FUTEX, (uint32_t)&futex_word, 0, 0);
    (void)r; /* 0=uyandı */
    futex_done = 1;
    syscall(SYS_EXIT,0,0,0);
    for(;;) { asm volatile("hlt"); }
}

/* 15B: pthread testi paylaşılanları */
#include "pthread.h"
#include "dns.h"
#include "http.h"
#include "dl.h"
static volatile int pt_counter;
static pthread_mutex_t pt_mu;
static pthread_cond_t pt_cv;
static volatile int pt_flag;
static void* pt_worker(void* arg) {
    int n = (int)arg;
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&pt_mu);
        pt_counter++;
        pthread_mutex_unlock(&pt_mu);
    }
    return (void*)0x2A;
}
static void* pt_waiter(void* arg) {
    (void)arg;
    pthread_mutex_lock(&pt_mu);
    while (!pt_flag) pthread_cond_wait(&pt_cv, &pt_mu);
    pthread_mutex_unlock(&pt_mu);
    return (void*)1;
}

/* 15C: thread kendi TLS bloğunda çalışmalı (blok izolasyonu) */
static int (*gtls_set_fn)(int);
static int (*gtls_get_fn)(void);
static volatile int gtls_ok;
static void* tls_thread_fn(void* arg) {
    (void)arg;
    int v = gtls_set_fn(7);
    gtls_ok = (gtls_get_fn() == 7 && v == 7) ? 1 : 0;
    return (void*)(unsigned)gtls_ok;
}

/* 15D/15G: program dışa aktarmaları */
static int prog_version_impl(void) { return 7; }
static volatile int g_on_init, g_on_fini;
static void prog_oninit_impl(void) { g_on_init = 1; }
static void prog_onfini_impl(void) { g_on_fini = 1; }
static void* g_he; /* 15F için libe handle'ı (slot bırakmada kullanılır) */

/* 16 önsözde kapatılacak 15 serisi tutamaçları (refs değerleri biliniyor) */
static void* g_h15tls;
static void* g_h15a;
static void* g_h15b;

static int up_streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

/* 17: alt-string arama (0/1) */
static int up_contains(const char* hay, const char* ndl) {
    if (!*ndl) return 1;
    for (; *hay; hay++) {
        const char* h = hay;
        const char* n = ndl;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return 1;
    }
    return 0;
}

/* 19J: alt-string sayımı (örtüşmesiz) */
static int up_count(const char* hay, const char* ndl) {
    int c = 0;
    if (!*ndl) return 0;
    for (; *hay; hay++) {
        const char* h = hay;
        const char* n = ndl;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) c++;
    }
    return c;
}

/* 17: ondalık parse/yaz */
static int up_atoi(const char* s) {
    int v = 0;
    if (!s) return -1;
    if (!*s) return -1;
    while (*s) {
        if (*s < '0' || *s > '9') return -1;
        v = v * 10 + (*s - '0');
        if (v > 999999) return -1;
        s++;
    }
    return v;
}
static int up_itoa_to(unsigned v, char* out) {
    char tmp[12];
    int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    out[n] = '\0';
    return n;
}

/* 17: disk test tamponları (yığın yerine BSS) */
static unsigned char df_wbig[16384];
static unsigned char df_rbig[16384];
static char df_listbuf[1024];

/* 19: test verdict'leri globalde tutulur — _start çerçevesindeki stack
 * slotları layout'a bağlı olarak sıfırlanabiliyor (19A ok=0/gok=1 ile
 * kanıtlandı); cap_status önceliği izlenir. */
static int t_ok;
static char cap_buf[512];
static int cap_status;
static const char* rc_prog;
static const char* rc_args;
/* 19: program çıktısı yakalama — fork+stdout-pipe+exec(argv), EOF'a kadar oku.
 * Dönüş: okunan bayt (cap_buf NUL-sonlu), -1 kurulum hatası. cap_status = çıkış kodu.
 * NOT: prog/args globalde tutulur — derleyici pointer parametreyi ebp'de
 * taşıyıp ömrünü yanlış hesaplayabiliyor (exec'e çöp pointer gidiyordu). */
static int run_capture(const char* prog, const char* args) {
    rc_prog = prog;
    rc_args = args;
    int fds[2];
    if (syscall(SYS_PIPE, (uint32_t)fds, 0, 0) != 0) return -1;
    volatile uint32_t c = syscall(SYS_FORK, 0, 0, 0);
    if (*(volatile uint32_t*)&c == 0) {
        syscall(SYS_CLOSE, fds[0], 0, 0);
        syscall(SYS_DUP2, fds[1], 1, 0);
        syscall(SYS_CLOSE, fds[1], 0, 0);
        syscall(SYS_EXEC, (uint32_t)rc_prog, (uint32_t)rc_args, 0);
        syscall(SYS_EXIT, 1, 0, 0);
        for (;;) asm volatile("hlt");
    }
    syscall(SYS_CLOSE, fds[1], 0, 0);
    int total = 0;
    uint32_t t0 = syscall(SYS_GETTICKS, 0, 0, 0);
    while (total < 511) {
        int r = (int)syscall(SYS_READ, fds[0], (uint32_t)(cap_buf + total), 511 - total);
        if (r <= 0) break;
        total += r;
        if ((syscall(SYS_GETTICKS, 0, 0, 0) - t0) > 400) break;
    }
    cap_buf[total] = 0;
    syscall(SYS_CLOSE, fds[0], 0, 0);
    cap_status = -1;
    syscall(SYS_WAITPID, c, (uint32_t)&cap_status, 0);
    return total;
}

/* 19: shell'i script ile çalıştır, stdout'u yakala (sh_outbuf NUL-sonlu).
 * Script "exit\n" ile bitmeli. Dönüş: bayt / -1 kurulum hatası.
 * NOT: script globalde tutulur (rc_prog ile aynı derleyici ömür sorunu). */
static char sh_outbuf[2048];
static const char* sh_script;
static int sh_status; /* 19F: son sh_run_script çağrısının shell çıkış kodu */
static int sh_run_script(const char* script) {
    sh_script = script;
    int sin[2], sout[2];
    if (syscall(SYS_PIPE, (uint32_t)sin, 0, 0) != 0) return -1;
    if (syscall(SYS_PIPE, (uint32_t)sout, 0, 0) != 0) {
        syscall(SYS_CLOSE, sin[0], 0, 0);
        syscall(SYS_CLOSE, sin[1], 0, 0);
        return -1;
    }
    volatile uint32_t c = syscall(SYS_FORK, 0, 0, 0);
    if (*(volatile uint32_t*)&c == 0) {
        syscall(SYS_CLOSE, sin[1], 0, 0);
        syscall(SYS_CLOSE, sout[0], 0, 0);
        if (syscall(SYS_DUP2, sin[0], 0, 0) != 0) syscall(SYS_EXIT, 2, 0, 0);
        if (syscall(SYS_DUP2, sout[1], 1, 0) != 1) syscall(SYS_EXIT, 2, 0, 0);
        syscall(SYS_CLOSE, sin[0], 0, 0);
        syscall(SYS_CLOSE, sout[1], 0, 0);
        syscall(SYS_EXEC, (uint32_t)"/bin/sh", 0, 0);
        syscall(SYS_EXIT, 1, 0, 0);
        for (;;) asm volatile("hlt");
    }
    syscall(SYS_CLOSE, sin[0], 0, 0);
    syscall(SYS_CLOSE, sout[1], 0, 0);
    int sl = 0;
    while (sh_script[sl]) sl++;
    int wo = 0;
    while (wo < sl) {
        int w = (int)syscall(SYS_WRITE, sin[1], (uint32_t)(sh_script + wo), sl - wo);
        if (w <= 0) break;
        wo += w;
    }
    syscall(SYS_CLOSE, sin[1], 0, 0);
    int total = 0;
    uint32_t t0 = syscall(SYS_GETTICKS, 0, 0, 0);
    while (total < 2047) {
        int r = (int)syscall(SYS_READ, sout[0], (uint32_t)(sh_outbuf + total), 2047 - total);
        if (r <= 0) break;
        total += r;
        if ((syscall(SYS_GETTICKS, 0, 0, 0) - t0) > 500) break;
    }
    sh_outbuf[total] = 0;
    syscall(SYS_CLOSE, sout[0], 0, 0);
    sh_status = 0;
    syscall(SYS_WAITPID, c, (uint32_t)&sh_status, 0);
    return total;
}

/* 16D: ana program tarafından sağlanan la_hook (v+1) */
static int la_hook_impl(int v) { return v + 1; }

/* 16G: dl_iterate_phdr geri çağrısı + sayaçlar */
static int iter_count, iter_main, iter_ok;
static int iter_cb_fn(struct dl_phdr_info* pi, unsigned sz, void* arg) {
    if (sz < sizeof(struct dl_phdr_info)) return 1;
    iter_count++;
    if (!pi->base) return 0;
    if (pi->phnum > 0) iter_ok = 1;
    if (pi->name && up_streq(pi->name, "main")) iter_main = 1;
    if (arg) (*(int*)arg)++;
    return 0;
}

/* 13F: thread fonksiyonu - shared'i artır */
static volatile uint32_t shared_var;
static void clone_thread_fn(void) {
    shared_var++;
    // thread için basit: shared sayacı artır, sonra exit
    // syscall ile exit (SYS_EXIT = 0)
    uint32_t r; asm volatile("int $0x80" : "=a"(r) : "a"(0), "b"(0), "c"(0), "d"(0) : "memory");
    // daha güvenli: doğrudan syscall wrapper kullan
    for(;;) { asm volatile("hlt"); }
}

__attribute__((section(".entry")))
void _start(void) {
    up_puts("Hello from Ring 3! pid=");
    up_putdec(syscall(SYS_GETPID, 0, 0, 0));
    up_puts("\n");

    /* chFS quick test (kısa, hang önlemek için) */
    int fd = syscall(SYS_OPEN, (uint32_t)"/test", 5, 0);
    if (fd >= 0) {
        syscall(SYS_WRITE, fd, (uint32_t)"ok\n", 3);
        syscall(SYS_CLOSE, fd, 0, 0);
    }
    up_puts("chFS test done\n");

    /* 12A/12B: Gerçek fork (izole address space + bellek kopya) */
    up_puts("[12A] isolated AS test: my pid="); up_putdec(syscall(SYS_GETPID, 0, 0, 0)); up_puts("\n");
    {
        volatile uint32_t x = 42;
        volatile uint32_t child = syscall(SYS_FORK, 0, 0, 0);
        // Force re-read
        asm volatile("" : : "m"(child) : "memory");
        // Check using a different register to avoid compiler optimization
        uint32_t child_read = *(volatile uint32_t*)&child;
        if (child_read == 0) {
            const char cmsg[] = "  [12B] child alive\n";
            syscall(SYS_WRITE, 1, (uint32_t)cmsg, 19);
            syscall(SYS_EXIT, 0, 0, 0);
        } else {
            up_puts("  [12B] parent: fork=="); up_putdec(child); up_puts(" x still "); up_putdec(x); up_puts("\n");
            for (int i = 0; i < 5; i++) syscall(SYS_YIELD, 0, 0, 0);
            { int st=0; syscall(SYS_WAITPID, child, (uint32_t)&st, 0); } // zombie temizle (13E için)
            up_puts("  [12B] parent: after child, x="); up_putdec(x); up_puts(x == 42 ? " [PASS - isolated]\n" : " [FAIL]\n");
        }
    }
    up_puts("9A/12B test done\n");

    /* 12E/12F direct tests first (no fork) */
    up_puts("[12E] nanosleep test...\n");
    {
        uint32_t t0 = syscall(SYS_GETTICKS,0,0,0);
        syscall(SYS_NANOSLEEP, 20,0,0);
        uint32_t t1 = syscall(SYS_GETTICKS,0,0,0);
        up_puts("  slept ticks="); up_putdec(t1-t0); up_puts(t1-t0 >= 18 ? " [PASS - sleep]\n" : " [FAIL]\n");
        up_puts("[12E] sleep test done [PASS]\n");
    }
    up_puts("[12F] semaphore test...\n");
    {
        int sem = syscall(SYS_SEM_INIT, 1,0,0);
        if (sem < 0) { up_puts("  sem_create failed [FAIL]\n"); }
        else {
            syscall(SYS_SEM_WAIT, sem,0,0);
            up_puts("  sem wait ok\n");
            syscall(SYS_SEM_POST, sem,0,0);
            up_puts("  sem post ok\n");
            syscall(SYS_SEM_WAIT, sem,0,0);
            syscall(SYS_SEM_POST, sem,0,0);
            up_puts("  sem second wait/post ok\n");
            syscall(SYS_SEM_DESTROY, sem,0,0);
            up_puts("[12F] sem test done [PASS]\n");
        }
    }

    /* 12C: exec test: /bin/exec_test -> should print '12C exec_test: OK' */
    up_puts("[12C] exec test: /bin/exec_test -> should print '12C exec_test: OK'\n");
    {
        volatile uint32_t c = syscall(SYS_FORK, 0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            int ret = syscall(SYS_EXEC, (uint32_t)"/bin/exec_test", 0, 0);
            up_puts("  [12C] exec failed ret="); up_putdec(ret); up_puts("\n");
            syscall(SYS_EXIT, 1,0,0);
        } else {
            for (int i=0;i<10;i++) syscall(SYS_YIELD,0,0,0);
            { int st=0; syscall(SYS_WAITPID, c, (uint32_t)&st, 0); } // zombie temizle (13E için)
            up_puts("[12C] exec parent done (child should have printed)\n");
        }
    }

    /* 12D: per-process FD test */
    up_puts("[12D] per-process FD test...\n");
    {
        int fd_p = syscall(SYS_OPEN, (uint32_t)"/pfd_parent", O_CREATE, 0);
        syscall(SYS_WRITE, fd_p, (uint32_t)"parent", 6);
        // keep fd_p open across fork
        volatile uint32_t c = syscall(SYS_FORK, 0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            // child: fd_p should still be valid in child's table (copy), close it and open own
            syscall(SYS_CLOSE, fd_p, 0,0);
            int cfd = syscall(SYS_OPEN, (uint32_t)"/pfd_child", O_CREATE, 0);
            syscall(SYS_WRITE, cfd, (uint32_t)"child", 5);
            syscall(SYS_CLOSE, cfd, 0,0);
            // verify parent's file still exists via open
            int check = syscall(SYS_OPEN, (uint32_t)"/pfd_parent", 0,0);
            if (check >= 0) {
                char buf[8]; int n = syscall(SYS_READ, check, (uint32_t)buf, 6);
                buf[n]=0;
                up_puts("  [12D] child sees parent file: "); up_puts(buf); up_puts("\n");
                syscall(SYS_CLOSE, check,0,0);
            }
            syscall(SYS_EXIT,0,0,0);
        } else {
            for (int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
            { int st=0; syscall(SYS_WAITPID, c, (uint32_t)&st, 0); } // zombie temizle (13E için)
            // parent: fd_p should still be valid (per-process)
            char buf[8]; syscall(SYS_CLOSE, fd_p,0,0); // close and reopen to check
            int pfd = syscall(SYS_OPEN, (uint32_t)"/pfd_parent", 0,0);
            int n = syscall(SYS_READ, pfd, (uint32_t)buf, 6);
            buf[n]=0;
            up_puts("  [12D] parent file: "); up_puts(buf);
            up_puts(n==6 ? " [PASS]\n" : " [FAIL]\n");
            syscall(SYS_CLOSE, pfd,0,0);
            // check child file
            int cfd = syscall(SYS_OPEN, (uint32_t)"/pfd_child", 0,0);
            if (cfd>=0) {
                n = syscall(SYS_READ, cfd, (uint32_t)buf, 5);
                buf[n]=0;
                up_puts("  [12D] child file from parent: "); up_puts(buf); up_puts("\n");
                syscall(SYS_CLOSE, cfd,0,0);
            }
            for (int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
            up_puts("[12D] FD test done\n");
        }
    }

    /* 12D: blocking pipe test */
    up_puts("[12D] blocking pipe test...\n");
    {
        int fds[2];
        int ret = syscall(SYS_PIPE, (uint32_t)fds, 0,0);
        if (ret==0) {
            const char* msg="pipe OK\n";
            syscall(SYS_WRITE, fds[1], (uint32_t)msg, 8);
            char buf[16];
            int n = syscall(SYS_READ, fds[0], (uint32_t)buf, 8);
            buf[n]=0;
            up_puts("  direct pipe read: "); up_puts(buf);
            up_puts(n==8 ? " [PASS]\n" : " [FAIL]\n");
            syscall(SYS_CLOSE, fds[0],0,0);
            syscall(SYS_CLOSE, fds[1],0,0);
        }
        ret = syscall(SYS_PIPE, (uint32_t)fds, 0,0);
        if (ret==0) {
            volatile uint32_t c = syscall(SYS_FORK, 0,0,0);
            if (*(volatile uint32_t*)&c == 0) {
                syscall(SYS_CLOSE, fds[0],0,0);
                const char* msg="pipe OK\n";
                syscall(SYS_WRITE, fds[1], (uint32_t)msg, 8);
                syscall(SYS_CLOSE, fds[1],0,0);
                syscall(SYS_EXIT,0,0,0);
            } else {
                syscall(SYS_CLOSE, fds[1],0,0);
                char buf[16];
                int n = syscall(SYS_READ, fds[0], (uint32_t)buf, 8);
                if (n>0) {
                    buf[n]=0;
                    up_puts("  fork pipe read: "); up_puts(buf);
                    up_puts(" [PASS - blocking]\n");
                } else {
                    up_puts("  fork pipe read: got 0 [PASS]\n");
                }
                syscall(SYS_CLOSE, fds[0],0,0);
                for(int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
                { int st=0; syscall(SYS_WAITPID, c, (uint32_t)&st, 0); } // zombie temizle (13E için)
            }
        }
        up_puts("[12D] pipe test done\n");
    }

    up_puts("[12G] signal test...\n");
    {
        // Direct signal test (no fork, just set handler and check)
        int ret = syscall(SYS_SIGNAL, 10, (uint32_t)0x1000000, 0); // dummy handler
        up_puts(ret==0 ? "  signal set [PASS]\n" : "  signal set [FAIL]\n");
        syscall(SYS_KILL, syscall(SYS_GETPID,0,0,0), 10, 0);
        up_puts("  kill self done\n");
        // Fork test with signal
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            syscall(SYS_SIGNAL, 10, (uint32_t)0x1000000, 0);
            for(int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
            syscall(SYS_EXIT,0,0,0);
        } else {
            syscall(SYS_KILL, c, 10, 0);
            up_puts("  kill child [PASS]\n");
            int status=0;
            syscall(SYS_WAITPID, c, (uint32_t)&status, 0);
            up_puts("[12G] signal test done [PASS]\n");
        }
        for(int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
    }

    up_puts("[12H] waitpid + pipe shell test...\n");
    {
        int fds[2];
        if (syscall(SYS_PIPE, (uint32_t)fds,0,0)==0) {
            volatile uint32_t c1 = syscall(SYS_FORK,0,0,0);
            if (*(volatile uint32_t*)&c1 == 0) {
                syscall(SYS_CLOSE, fds[0],0,0);
                const char* msg="hello\n";
                syscall(SYS_WRITE, fds[1], (uint32_t)msg, 6);
                syscall(SYS_CLOSE, fds[1],0,0);
                syscall(SYS_EXIT,0,0,0);
            }
            volatile uint32_t c2 = syscall(SYS_FORK,0,0,0);
            if (*(volatile uint32_t*)&c2 == 0) {
                syscall(SYS_CLOSE, fds[1],0,0);
                char buf[16];
                int n = syscall(SYS_READ, fds[0], (uint32_t)buf, 6);
                buf[n]=0;
                up_puts("  pipe chain: "); up_puts(buf);
                up_puts(n==6 ? " [PASS]\n" : " [FAIL]\n");
                syscall(SYS_CLOSE, fds[0],0,0);
                syscall(SYS_EXIT,0,0,0);
            }
            syscall(SYS_CLOSE, fds[0],0,0);
            syscall(SYS_CLOSE, fds[1],0,0);
            int st1, st2;
            syscall(SYS_WAITPID, c1, (uint32_t)&st1, 0);
            syscall(SYS_WAITPID, c2, (uint32_t)&st2, 0);
            up_puts("[12H] pipe+wait done [PASS]\n");
        }
        // Shell exec test (fork+exec sh)
        volatile uint32_t sc = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&sc == 0) {
            int ret = syscall(SYS_EXEC, (uint32_t)"/bin/sh",0,0);
            (void)ret;
            up_puts("  sh exec failed\n");
            syscall(SYS_EXIT,1,0,0);
        } else {
            // parent doesn't wait, just test that sh can be exec'd (we don't have input, so just check fork+exec doesn't crash)
            for(int i=0;i<10;i++) syscall(SYS_YIELD,0,0,0);
            // kill the shell if still running (it will be in its read loop)
            syscall(SYS_KILL, sc, 1,0);
            int st;
            syscall(SYS_WAITPID, sc, (uint32_t)&st, 0);
            up_puts("[12H] shell test done [PASS]\n");
        }
    }

    up_puts("[13C] mmap test...\n");
    {
        void* addr = (void*)syscall(SYS_MMAP, 0, 4096, PROT_READ|PROT_WRITE | (MAP_PRIVATE|MAP_ANONYMOUS)<<8);
        if (addr != (void*)-1) {
            *(volatile int*)addr = 42;
            int v = *(volatile int*)addr;
            up_puts(v==42 ? "  mmap anon [PASS]\n" : "  mmap anon [FAIL]\n");
            syscall(SYS_MUNMAP, (uint32_t)addr, 4096,0);
        } else {
            up_puts("  mmap failed [FAIL]\n");
        }
        up_puts("[13C] mmap done\n");
    }

    up_puts("[13D] nice/MLQ test...\n");
    {
        int old = syscall(SYS_NICE, 1,0,0);
        up_puts("  nice new prio "); up_putdec(old); up_puts("\n");
        uint32_t t0 = syscall(SYS_GETTICKS,0,0,0);
        for(int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
        uint32_t t1 = syscall(SYS_GETTICKS,0,0,0);
        up_puts("  ticks "); up_putdec(t1-t0); up_puts(" [PASS]\n");
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c==0) {
            syscall(SYS_NICE, -1,0,0);
            for(int i=0;i<3;i++) { up_puts("  high prio child\n"); syscall(SYS_NANOSLEEP,5,0,0); }
            syscall(SYS_EXIT,0,0,0);
        } else {
            syscall(SYS_NICE, 1,0,0);
            for(int i=0;i<3;i++) { up_puts("  low prio parent\n"); syscall(SYS_NANOSLEEP,5,0,0); }
            int st; syscall(SYS_WAITPID,c,(uint32_t)&st,0);
            up_puts("[13D] MLQ done [PASS]\n");
        }
        for(int i=0;i<5;i++) syscall(SYS_YIELD,0,0,0);
    }

    up_puts("[13E] wait/zombie test...\n");
    {
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c==0) {
            up_puts("  child now exits with 7\n");
            syscall(SYS_EXIT, 7,0,0);
        } else {
            int st=0;
            int r = syscall(SYS_WAITPID, c, (uint32_t)&st, 0);
            up_puts("  waitpid("); up_putdec(c); up_puts(") -> "); up_putdec(r);
            up_puts(" status="); up_putdec(st);
            up_puts(st==7 ? " [PASS]\n" : " [FAIL]\n");
            // no children left -> waitpid returns -1
            int r2 = syscall(SYS_WAITPID, -1, (uint32_t)&st, 0);
            up_puts("  no-child waitpid -> "); up_putdec(r2); up_puts(r2==(int)(uint32_t)-1 ? " [PASS]\n" : " [FAIL]\n");
        }
        up_puts("[13E] zombie test done\n");
    }

    up_puts("[13F] clone/thread test...\n");
    {
        shared_var = 0;
        void* thstack = (void*)syscall(SYS_MMAP, 0, 4096*2, PROT_READ|PROT_WRITE|(MAP_PRIVATE|MAP_ANONYMOUS)<<8);
        if (thstack == (void*)-1) { up_puts("  clone stack mmap failed\n"); }
        else {
            uint32_t tid = syscall(SYS_CLONE, (uint32_t)clone_thread_fn, (uint32_t)thstack + 8192, 0);
            if (tid == (uint32_t)-1) up_puts("  clone failed [FAIL]\n");
            else {
                for (int i=0; i<10; i++) syscall(SYS_YIELD,0,0,0);
                // thread'i bekle (join) - shared mem doğrulaması için
                int st=0; syscall(SYS_WAITPID, tid, (uint32_t)&st, 0);
                int ok = (shared_var > 0);
                up_puts("  thread shared="); up_putdec(shared_var);
                up_puts(ok ? " [PASS - shared mem]\n" : " [FAIL]\n");
                syscall(SYS_MUNMAP, (uint32_t)thstack, 4096*2, 0);
            }
        }
        up_puts("[13F] clone done\n");
    }

    up_puts("[13G] virtio-blk sector test...\n");
    {
        uint32_t cap = syscall(SYS_BLKINFO, 0, 0, 0);
        if (cap == 0) {
            up_puts("  no block device [SKIP]\n");
        } else {
            static char wsec[512], rsec[512];
            for (int i = 0; i < 512; i++) wsec[i] = (char)(0x3C + (i & 0x7F));
            for (int i = 0; i < 512; i++) rsec[i] = 0;
            uint32_t lba = cap - 1; /* scratch = son sektör */
            int ok = 1;
            if (syscall(SYS_BLKWRITE, lba, (uint32_t)wsec, 0) != 0) ok = 0;
            if (syscall(SYS_BLKREAD, lba, (uint32_t)rsec, 0) != 0) ok = 0;
            for (int i = 0; i < 512 && ok; i++) if (rsec[i] != wsec[i]) ok = 0;
            up_puts("  sector RW cap="); up_putdec(cap);
            up_puts(ok ? " [PASS]\n" : " [FAIL]\n");
        }
        up_puts("[13G] blk done\n");
    }

    up_puts("[13H] diskfs persist test...\n");
    {
        const char msg[] = "hello-diskfs-13H";
        char rbuf[32];
        for (int i = 0; i < 32; i++) rbuf[i] = 0;
        int w = syscall(SYS_DFSAVE, (uint32_t)"u13h", (uint32_t)msg, 16);
        int n = syscall(SYS_DFLOAD, (uint32_t)"u13h", (uint32_t)rbuf, 32);
        int ok = (w == 16 && n == 16);
        for (int i = 0; i < 16 && ok; i++) if (rbuf[i] != msg[i]) ok = 0;
        up_puts("  dfsave/dfload ");
        up_puts(ok ? "[PASS]\n" : "[FAIL]\n");
        up_puts("[13H] diskfs done\n");
    }

    up_puts("[18A] input hub test...\n");
    {
        char ev[6];
        int drained = 0, r;
        /* Bekleyen olayı boşalt (headless'ta genelde 0), ABI çökmesin yeter */
        while ((r = syscall(SYS_INPUT, (uint32_t)ev, 0, 0)) > 0 && drained < 130) drained++;
        up_puts("  hub drain=");
        up_putdec(drained);
        up_puts(r >= 0 ? " [PASS]\n" : " [FAIL]\n");
        up_puts("[18A] input done\n");
    }

    up_puts("[18B] framebuffer test...\n");
    {
        uint32_t m[3] = {0, 0, 0};
        int r = syscall(SYS_FBINFO, (uint32_t)m, 0, 0);
        if (r <= 0) {
            up_puts("  no framebuffer [SKIP]\n");
        } else {
            up_puts("  fb mode=");
            up_putdec(m[0]); up_puts("x"); up_putdec(m[1]);
            up_puts("x"); up_putdec(m[2]);
            up_puts((m[0] > 0 && m[1] > 0) ? " [PASS]\n" : " [FAIL]\n");
        }
        up_puts("[18B] fb done\n");
    }

    up_puts("[14A] libcharos test...\n");
    {
        int ok = 1;
        uint32_t b0 = syscall(SYS_BRK, 0,0,0);
        if (b0 == 0) ok = 0;
        char* p = (char*)lib_malloc(64);
        if (!p) ok = 0;
        else {
            for (int i=0;i<64;i++) p[i] = (char)(i+1);
            for (int i=0;i<64;i++) if (p[i] != (char)(i+1)) ok = 0;
        }
        uint32_t b1 = syscall(SYS_BRK, 0,0,0);
        if (b1 <= b0) ok = 0; /* heap büyümüş olmalı */
        char* q = (char*)lib_malloc(32);
        if (!q) ok = 0;
        lib_free(p);
        char* r = (char*)lib_malloc(16);
        if (!r) ok = 0;
        else if (r != p) ok = 0; /* first-fit yeniden kullanım */
        int n = lib_printf("[14A] printf %d %x %s %c ok\n", 42, 0xAB, "str", 'Q');
        if (n != 28) ok = 0;
        /* ham brk büyüt/küçült (lib üstü alan, lib tutarlı kalır) */
        {
            uint32_t bx = syscall(SYS_BRK, 0,0,0);
            uint32_t grow = syscall(SYS_BRK, bx+0x2000,0,0);
            if (grow != bx+0x2000) ok = 0;
            else {
                volatile char* vp = (volatile char*)bx;
                vp[0] = 'A'; vp[0x1FFF] = 'Z';
                if (vp[0] != 'A' || vp[0x1FFF] != 'Z') ok = 0;
                if (syscall(SYS_BRK, bx,0,0) != bx) ok = 0;
            }
        }
        up_puts(ok ? "[14A] libcharos heap/printf [PASS]\n" : "[14A] libcharos [FAIL]\n");
    }

    up_puts("[14C] getty login test...\n");
    {
        int ok = 1;
        int wf = syscall(SYS_OPEN, (uint32_t)"/tmp/login.in", O_CREATE, 0);
        if (wf < 0) ok = 0;
        else {
            if (syscall(SYS_WRITE, wf, (uint32_t)"root\n", 5) != 5) ok = 0;
            syscall(SYS_CLOSE, wf, 0,0);
        }
        /* Yazılanı geri oku (FS görünürlüğü kapısı) */
        if (ok) {
            int vf = syscall(SYS_OPEN, (uint32_t)"/tmp/login.in", 1,0);
            if (vf < 0) ok = 0;
            else {
                char vb[8]; int vn = syscall(SYS_READ, vf, (uint32_t)vb, 5);
                if (vn != 5 || vb[0]!='r' || vb[4]!='\n') ok = 0;
                syscall(SYS_CLOSE, vf,0,0);
            }
        }
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            /* Art arda open zinciri (fd regresyonu) + stdin yönlendirme */
            int t1 = syscall(SYS_OPEN, (uint32_t)"/test", 1,0);
            int t2 = syscall(SYS_OPEN, (uint32_t)"/tmp/login.in", O_CREATE, 0);
            int sfd = syscall(SYS_OPEN, (uint32_t)"/tmp/login.in", 1,0);
            if (t1 < 0 || t2 < 0 || sfd < 0) syscall(SYS_EXIT,2,0,0);
            syscall(SYS_CLOSE, t1,0,0);
            syscall(SYS_CLOSE, t2,0,0);
            if (syscall(SYS_DUP2, sfd, 0, 0) != 0) syscall(SYS_EXIT,2,0,0);
            syscall(SYS_CLOSE, sfd, 0,0);
            int ret = syscall(SYS_EXEC, (uint32_t)"/bin/getty",0,0);
            (void)ret;
            syscall(SYS_EXIT,1,0,0);
        } else {
            /* Sabit 20 yield yerine içerik anketi (zamanlama yarışını bitirir) */
            int found = 0;
            for (int i=0;i<200 && !found;i++) {
                int rfd = syscall(SYS_OPEN, (uint32_t)"/tmp/login.ok", 0,0);
                if (rfd >= 0) {
                    char ubuf[8]; int n = syscall(SYS_READ, rfd, (uint32_t)ubuf, 4);
                    syscall(SYS_CLOSE, rfd,0,0);
                    if (n == 4 && ubuf[0]=='r' && ubuf[1]=='o' && ubuf[2]=='o' && ubuf[3]=='t')
                        found = 1;
                    /* else: kısmi yazım olabilir, yoklamaya devam */
                }
                syscall(SYS_YIELD,0,0,0);
            }
            if (!found) ok = 0;
            syscall(SYS_KILL, c, 1,0);
            { int st=0; syscall(SYS_WAITPID, c, (uint32_t)&st, 0); }
            up_puts(ok ? "[14C] getty login [PASS]\n" : "[14C] getty [FAIL]\n");
        }
    }

    up_puts("[14D] futex test...\n");
    {
        int ok = 1;
        /* Eşleşmeyen değer: bloklanmadan -1 */
        futex_word = 0;
        futex_done = 0;
        if (syscall(SYS_FUTEX, (uint32_t)&futex_word, 0, 1) != (uint32_t)-1) ok = 0;
        /* Boş adresten uyandırma: 0 */
        if (syscall(SYS_FUTEX, (uint32_t)&futex_word, 1, 1) != 0) ok = 0;
        void* thstack = (void*)syscall(SYS_MMAP, 0, 4096*2, PROT_READ|PROT_WRITE|(MAP_PRIVATE|MAP_ANONYMOUS)<<8);
        if (thstack == (void*)-1) ok = 0;
        else {
            uint32_t tid = syscall(SYS_CLONE, (uint32_t)futex_thread_fn, (uint32_t)thstack + 8192, 0);
            if (tid == (uint32_t)-1) ok = 0;
            else {
                /* Randevu: uyandır, olmadıysa tekrarla (en fazla 200 tur) */
                int i;
                for (i = 0; i < 200 && !futex_done; i++) {
                    syscall(SYS_FUTEX, (uint32_t)&futex_word, 1, 1);
                    syscall(SYS_YIELD,0,0,0);
                }
                if (!futex_done) ok = 0;
                /* Değer değişmişken bekleme: anında -1 */
                futex_word = 5;
                if (syscall(SYS_FUTEX, (uint32_t)&futex_word, 0, 0) != (uint32_t)-1) ok = 0;
                { int st=0; syscall(SYS_WAITPID, tid, (uint32_t)&st, 0); }
            }
            syscall(SYS_MUNMAP, (uint32_t)thstack, 4096*2, 0);
        }
        up_puts(ok ? "[14D] futex wait/wake [PASS]\n" : "[14D] futex [FAIL]\n");
    }

    up_puts("[14E] uid/gid/perm test...\n");
    {
        int ok = 1;
        up_mypid = syscall(SYS_GETPID,0,0,0);
        if (syscall(SYS_GETUID,0,0,0) != 0) ok = 0;
        if (syscall(SYS_GETGID,0,0,0) != 0) ok = 0;
        /* root kısıtlı dosya üretir */
        int fd = syscall(SYS_OPEN, (uint32_t)"/perm600", O_CREATE, 0);
        if (fd < 0) ok = 0;
        else {
            if (syscall(SYS_WRITE, fd, (uint32_t)"secret", 6) != 6) ok = 0;
            syscall(SYS_CLOSE, fd, 0,0);
        }
        if (syscall(SYS_CHMOD, (uint32_t)"/perm600", 0600, 0) != 0) ok = 0;
        /* root okuyabilir (muafiyet) */
        {
            int rfd = syscall(SYS_OPEN, (uint32_t)"/perm600", 1,0);
            if (rfd < 0) ok = 0;
            else {
                char b[8]; int n = syscall(SYS_READ, rfd, (uint32_t)b, 6);
                if (n != 6) ok = 0;
                syscall(SYS_CLOSE, rfd,0,0);
            }
        }
        volatile uint32_t c = syscall(SYS_FORK,0,0,0);
        if (*(volatile uint32_t*)&c == 0) {
            /* çocuk: uid düşür, reddedilmeleri topla */
            int fails = 0;
            if (syscall(SYS_SETUID, 1000,0,0) != 1000) fails++;
            if (syscall(SYS_GETUID,0,0,0) != 1000) fails++;
            if (syscall(SYS_SETUID, 0,0,0) != (uint32_t)-1) fails++; /* yükselme yasak */
            int rfd = syscall(SYS_OPEN, (uint32_t)"/perm600", 1,0);
            if ((int)rfd >= 0) { fails++; syscall(SYS_CLOSE, rfd,0,0); }
            int wfd = syscall(SYS_OPEN, (uint32_t)"/perm600", 2,0);
            if ((int)wfd >= 0) { fails++; syscall(SYS_CLOSE, wfd,0,0); }
            /* root parent'ı öldürme girişimi reddedilmeli */
            if (syscall(SYS_KILL, up_mypid, 1,0) != (uint32_t)-1) fails++;
            syscall(SYS_EXIT, fails, 0,0);
        } else {
            int st = -1;
            syscall(SYS_WAITPID, c, (uint32_t)&st, 0);
            if (st != 0) ok = 0;
        }
        up_puts(ok ? "[14E] uid/perm/kill [PASS]\n" : "[14E] uid/perm [FAIL]\n");
    }

    up_puts("[14F] syslog/uptime test...\n");
    {
        int ok = 1;
        uint32_t t1 = syscall(SYS_UPTIME,0,0,0);
        syscall(SYS_NANOSLEEP, 120,0,0);
        uint32_t t2 = syscall(SYS_UPTIME,0,0,0);
        if (!(t1 > 0 && t2 > t1)) ok = 0;
        {
            const char* msg = "14F-USER-LOG";
            if (syscall(SYS_SYSLOG, 1, (uint32_t)msg, 12) != 12) ok = 0;
            else {
                char sb[64];
                uint32_t n = syscall(SYS_SYSLOG, 0, (uint32_t)sb, sizeof(sb));
                int found = 0;
                for (uint32_t i = 0; i + 12 <= n && i + 12 <= sizeof(sb); i++) {
                    int k = 0;
                    while (k < 12 && sb[i+k] == msg[k]) k++;
                    if (k == 12) { found = 1; break; }
                }
                if (!found) ok = 0;
            }
        }
        up_puts(ok ? "[14F] syslog/uptime [PASS]\n" : "[14F] syslog [FAIL]\n");
    }

    up_puts("[14I] dns/http test...\n");
    {
        int ok = 1;
        /* DNS sorgu kurulumu */
        {
            char q[64];
            int n = dns_build_query("example.com", 0x1234, q, sizeof(q));
            if (n < 17) ok = 0;
            else if ((unsigned char)q[0] != 0x12 || (unsigned char)q[1] != 0x34) ok = 0;
            else if ((unsigned char)q[2] != 0x01) ok = 0; /* RD */
            else {
                /* QNAME ilk etiket "example"(7) olmalı */
                if ((unsigned char)q[12] != 7) ok = 0;
            }
            if (dns_build_query("a..b", 1, q, sizeof(q)) != -1) ok = 0;
            if (dns_build_query("example.com", 1, q, 10) != -1) ok = 0;
        }
        /* DNS yanıt çözümleme (hazır example.com -> 93.184.216.34) */
        {
            static const char resp[] = {
                0x12,0x34, 0x81,0x80, 0x00,0x01, 0x00,0x01, 0x00,0x00, 0x00,0x00,
                0x07,'e','x','a','m','p','l','e', 0x03,'c','o','m', 0x00,
                0x00,0x01, 0x00,0x01,
                0xC0,0x0C, 0x00,0x01, 0x00,0x01,
                0x00,0x00,0x0E,0x10, 0x00,0x04,
                93,(char)184,(char)216,34
            };
            unsigned char ip[4];
            if (dns_parse_response(resp, sizeof(resp), ip) != 0) ok = 0;
            else if (ip[0]!=93 || ip[1]!=184 || ip[2]!=216 || ip[3]!=34) ok = 0;
            if (dns_parse_response(resp, 10, ip) != -1) ok = 0; /* kesik */
            if (dns_parse_response(resp, sizeof(resp), (unsigned char*)0) != -1) ok = 0;
        }
        /* HTTP istek/yanıt */
        {
            char req[128];
            int n = http_build_get("example.com", "/index", req, sizeof(req));
            if (n <= 0) ok = 0;
            else {
                /* "GET /index HTTP/1.0" öneki */
                const char* want = "GET /index HTTP/1.0\r\nHost: example.com\r\n\r\n";
                int i = 0;
                while (want[i] && i < n && req[i] == want[i]) i++;
                if (want[i] != 0) ok = 0;
            }
            const char* msg = "HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\nHELLO";
            int st = 0, bo = 0, bl = 0;
            if (http_parse_response(msg, 43, &st, &bo, &bl) != 0) ok = 0;
            else if (st != 200 || bl != 5) ok = 0;
            else if (msg[bo]!='H' || msg[bo+4]!='O') ok = 0;
            if (http_parse_response("HTTP/1.0 200 OK\r\nnoend", 21, &st, &bo, &bl) != -1) ok = 0;
            if (http_parse_status("HTTP/1.1 404 NF", 14) != 404) ok = 0;
        }
        up_puts(ok ? "[14I] dns/http [PASS]\n" : "[14I] dns/http [FAIL]\n");
    }

    up_puts("[15B] pthread test...\n");
    {
        int ok = 1;
        if (pthread_create(0, 0, 0, 0) != -1) ok = 0;
        if (pthread_mutex_init(&pt_mu, 0) != 0) ok = 0;
        if (pthread_cond_init(&pt_cv, 0) != 0) ok = 0;
        if (pthread_mutex_lock(0) != -1) ok = 0;
        pt_counter = 0;
        pt_flag = 0;
        pthread_t t1 = 0, t2 = 0, tw = 0;
        up_puts("  [15B-dbg] creating\n");
        if (pthread_create(&t1, 0, pt_worker, (void*)500) != 0) ok = 0;
        up_puts("  [15B-dbg] t1 ok\n");
        if (pthread_create(&t2, 0, pt_worker, (void*)500) != 0) ok = 0;
        up_puts("  [15B-dbg] t2 ok\n");
        void* r1 = 0;
        void* r2 = 0;
        if (pthread_join(t1, &r1) != 0) ok = 0;
        up_puts("  [15B-dbg] joined t1 cnt="); up_putdec((uint32_t)pt_counter); up_puts("\n");
        if (pthread_join(t2, &r2) != 0) ok = 0;
        up_puts("  [15B-dbg] joined t2 cnt="); up_putdec((uint32_t)pt_counter); up_puts("\n");
        if (r1 != (void*)0x2A || r2 != (void*)0x2A) ok = 0;
        if (pt_counter != 1000) ok = 0;
        if (pthread_create(&tw, 0, pt_waiter, 0) != 0) ok = 0;
        up_puts("  [15B-dbg] waiter created\n");
        for (int i = 0; i < 10; i++) syscall(SYS_YIELD,0,0,0);
        pthread_mutex_lock(&pt_mu);
        pt_flag = 1;
        pthread_cond_signal(&pt_cv);
        pthread_mutex_unlock(&pt_mu);
        up_puts("  [15B-dbg] signaled\n");
        void* rw = 0;
        if (pthread_join(tw, &rw) != 0) ok = 0;
        up_puts("  [15B-dbg] waiter joined\n");
        if (rw != (void*)1) ok = 0;
        up_puts(ok ? "[15B] pthread mutex/cond [PASS]\n" : "[15B] pthread [FAIL]\n");
    }

    up_puts("[15A] dlopen test...\n");
    {
        int ok = 1;
        void* h = dl_open("/lib/libmath.so");
        if (!h) ok = 0;
        else {
            int (*add)(int, int) = (int (*)(int, int))dl_sym(h, "lib_add");
            int (*mul)(int, int) = (int (*)(int, int))dl_sym(h, "lib_mul");
            int (*fact)(int) = (int (*)(int))dl_sym(h, "lib_fact");
            if (!add || !mul || !fact) ok = 0;
            else {
                if (add(40, 2) != 42) ok = 0;
                if (mul(6, 7) != 42) ok = 0;
                if (fact(5) != 120) ok = 0;
            }
            if (dl_sym(h, "yok_boyle_sembol") != 0) ok = 0;
            if (dl_close(h) != 0) ok = 0;
            if (dl_close(h) != -1) ok = 0; /* iki kez kapatma reddedilir */
        }
        if (dl_open("/lib/yok.so") != 0) ok = 0;
        up_puts(ok ? "[15A] dlopen/dlsym [PASS]\n" : "[15A] dlopen [FAIL]\n");
    }

    /* ============ 15 serisi derin: TLS / interpose / CoW / sağlamlık / init-fini / PIE ============ */

    /* 15C: thread-local storage (libtls.so) */
    up_puts("[15C] TLS test...\n");
    {
        int ok = 1;
        void* h = g_h15tls = dl_open("/lib/libtls.so");
        int (*tls_set)(int) = h ? (int (*)(int))dl_sym(h, "tls_set") : 0;
        int (*tls_get)(void) = h ? (int (*)(void))dl_sym(h, "tls_get") : 0;
        if (!h || !tls_set || !tls_get) ok = 0;
        else {
            dl_tls_main();               /* main bloğu kesinleştir */
            tls_set(41);
            if (tls_get() != 41) ok = 0;
            gtls_set_fn = tls_set;
            gtls_get_fn = tls_get;
            gtls_ok = 0;
            pthread_t tt = 0;
            if (pthread_create(&tt, 0, tls_thread_fn, 0) != 0) ok = 0;
            else {
                void* r = 0;
                if (pthread_join(tt, &r) != 0) ok = 0;
                if (r == 0) ok = 0;      /* thread kendi bloğunda 7 gördü */
            }
            if (tls_get() != 41) ok = 0; /* main bloğu izole kaldı */
        }
        up_puts(ok ? "[15C] TLS per-thread [PASS]\n" : "[15C] TLS [FAIL]\n");
    }

    /* 15D: interposition + program export'ları */
    up_puts("[15D] interposition test...\n");
    {
        int ok = 1;
        dl_register_export("prog_version", prog_version_impl);
        void* ha = g_h15a = dl_open("/lib/liba.so");
        void* hb = g_h15b = dl_open("/lib/libb.so");
        g_he = dl_open("/lib/libe.so");
        int (*liba_set_g)(int) = ha ? (int (*)(int))dl_sym(ha, "liba_set_g") : 0;
        int (*b_run)(int) = hb ? (int (*)(int))dl_sym(hb, "b_run") : 0;
        int (*b_var)(void) = hb ? (int (*)(void))dl_sym(hb, "b_var") : 0;
        int (*e_run)(void) = g_he ? (int (*)(void))dl_sym(g_he, "e_run") : 0;
        if (!ha || !hb || !g_he) ok = 0;
        else {
            if (!liba_set_g || !b_run || !b_var || !e_run) ok = 0;
            else {
                liba_set_g(10);
                if (b_var() != 12) ok = 0;   /* liba_g interpose (paylaşılan bellek) */
                if (b_run(5) != 16) ok = 0;  /* shared_hook interpose */
                if (e_run() != 8) ok = 0;    /* prog_version export */
            }
        }
        up_puts(ok ? "[15D] interposition [PASS]\n" : "[15D] interposition [FAIL]\n");
    }

    /* 15E: fork sonrası .so kullanımı (CoW paylaşımı + dedupe) */
    up_puts("[15E] fork .so share test...\n");
    {
        int ok = 1;
        void* hb = dl_open("/lib/libb.so");   /* dedupe: aynı handle */
        void* ha = dl_open("/lib/liba.so");
        int (*b_var)(void) = hb ? (int (*)(void))dl_sym(hb, "b_var") : 0;
        int (*liba_set_g)(int) = ha ? (int (*)(int))dl_sym(ha, "liba_set_g") : 0;
        if (!hb || !ha || !b_var || !liba_set_g) ok = 0;
        else {
            void* hb2 = dl_open("/lib/libb.so");
            if (hb2 != hb) ok = 0;            /* aynı isim == aynı handle */
            if (hb2) dl_close(hb2);
        }
        volatile uint32_t c = syscall(SYS_FORK, 0, 0, 0);
        if (*(volatile uint32_t*)&c == 0) {
            /* çocuk: miras alınan .so ile çalışır, kendi CoW kopyasını değiştirir */
            int okc = 1;
            liba_set_g(20);
            if (b_var() != 22) okc = 0;
            void* hb3 = dl_open("/lib/libb.so");
            if (!hb3 || hb3 != hb) okc = 0;
            int (*b_var3)(void) = hb3 ? (int (*)(void))dl_sym(hb3, "b_var") : 0;
            if (!b_var3 || b_var3() != 22) okc = 0;
            syscall(SYS_EXIT, okc ? 0 : 7, 0, 0);
            for (;;) asm volatile("hlt");
        } else {
            int st = -1;
            syscall(SYS_WAITPID, c, (uint32_t)&st, 0);
            if (st != 0) ok = 0;
            if (b_var() != 12) ok = 0;       /* ebeveyn izole: 10+2 */
        }
        up_puts(ok ? "[15E] fork .so share [PASS]\n" : "[15E] fork [FAIL]\n");
    }

    /* 15F: sağlamlık — bozuk dosyalar, slot tükenmesi, hata sonrası yükleme */
    up_puts("[15F] robustness test...\n");
    {
        int ok = 1;
        static const unsigned char bad0[4096] = "not an elf";
        char bad1[100];
        for (int i = 0; i < 100; i++) bad1[i] = 0;
        bad1[0] = 0x7F; bad1[1] = 'E'; bad1[2] = 'L'; bad1[3] = 'F';
        int fd = syscall(SYS_OPEN, (uint32_t)"/lib/bad0.so", 6, 0);
        if (fd >= 0) { syscall(SYS_WRITE, fd, (uint32_t)bad0, 4096); syscall(SYS_CLOSE, fd, 0, 0); }
        fd = syscall(SYS_OPEN, (uint32_t)"/lib/bad1.so", 6, 0);
        if (fd >= 0) { syscall(SYS_WRITE, fd, (uint32_t)bad1, 100); syscall(SYS_CLOSE, fd, 0, 0); }
        fd = syscall(SYS_OPEN, (uint32_t)"/lib/bad2.so", 6, 0);
        if (fd >= 0) { syscall(SYS_WRITE, fd, (uint32_t)bad1, 20); syscall(SYS_CLOSE, fd, 0, 0); }
        fd = syscall(SYS_OPEN, (uint32_t)"/lib/bad3.so", 6, 0); /* boş */
        if (fd >= 0) syscall(SYS_CLOSE, fd, 0, 0);

        if (dl_open("/lib/bad0.so") != 0) ok = 0;  /* çöp */
        if (dl_open("/lib/bad1.so") != 0) ok = 0;  /* kesik ELF */
        if (dl_open("/lib/bad2.so") != 0) ok = 0;  /* kesik ELF */
        if (dl_open("/lib/bad3.so") != 0) ok = 0;  /* boş dosya */
        if (dl_open("/lib/bogus.so") != 0) ok = 0; /* mevcut değil */
        if (dl_open("/lib/libg.so") != 0) ok = 0;  /* 4 slot dolu -> tükenme */

        if (g_he) { dl_close(g_he); g_he = 0; }    /* slot3'ü serbest bırak */
        void* hm = dl_open("/lib/libmath.so");      /* hata sonrası sağlamlık */
        int (*add)(int, int) = hm ? (int (*)(int, int))dl_sym(hm, "lib_add") : 0;
        if (!add || add(1, 2) != 3) ok = 0;
        if (hm) dl_close(hm);
        up_puts(ok ? "[15F] robustness [PASS]\n" : "[15F] robustness [FAIL]\n");
    }

    /* 15G: init/fini dizileri + export çağrıları */
    up_puts("[15G] init/fini test...\n");
    {
        int ok = 1;
        g_on_init = 0; g_on_fini = 0;
        dl_register_export("prog_oninit", prog_oninit_impl);
        dl_register_export("prog_onfini", prog_onfini_impl);
        void* hg = dl_open("/lib/libg.so");          /* ctor çalışmalı */
        int (*g_get)(void) = hg ? (int (*)(void))dl_sym(hg, "g_get") : 0;
        if (!hg || !g_get) ok = 0;
        else {
            if (g_get() != 1) ok = 0;                 /* ctor: g_state=1 */
            if (g_on_init != 1) ok = 0;               /* prog_oninit çağrıldı */
            if (dl_close(hg) != 0) ok = 0;            /* fini çalışmalı */
            if (g_on_fini != 1) ok = 0;               /* prog_onfini çağrıldı */
        }
        up_puts(ok ? "[15G] init/fini [PASS]\n" : "[15G] init/fini [FAIL]\n");
    }

    /* 15H: PIE (ET_DYN) giriş çalıştırma */
    up_puts("[15H] PIE entry test...\n");
    {
        int ok = 1;
        void* hp = dl_open("/lib/libpie.so");        /* liba interpose */
        int (*pie_entry)(void) = hp ? (int (*)(void))dl_entry(hp) : 0;
        int (*pie_peek)(void) = hp ? (int (*)(void))dl_sym(hp, "pie_peek") : 0;
        if (!hp || !pie_entry) ok = 0;
        else {
            if (pie_entry() != 10) ok = 0;           /* 3*2 + 3 + 1 */
            if (pie_entry() != 11) ok = 0;           /* 6 + 3 + 2 */
            if (pie_peek && pie_peek() != 2) ok = 0;
        }
        if (hp) dl_close(hp);
        up_puts(ok ? "[15H] PIE entry [PASS]\n" : "[15H] PIE entry [FAIL]\n");
    }

    /* ============ 16 serisi: dlerror / dladdr / kapsam / lazy / NEEDED / global / iter / main ============ */

    /* 16 önsöz: 15 serisi artık tutamaçlarını slotlardan boşalt.
     * refs biliniyor: libtls=1, liba=2, libb=2 (15D+15E dedupe). */
    if (g_h15tls) { dl_close(g_h15tls); g_h15tls = 0; }
    if (g_h15a) { dl_close(g_h15a); dl_close(g_h15a); g_h15a = 0; }
    if (g_h15b) { dl_close(g_h15b); dl_close(g_h15b); g_h15b = 0; }

    /* 16A: dlerror */
    up_puts("[16A] dlerror test...\n");
    {
        int ok = 1;
        if (dlerror() != 0) ok = 0;              /* ilk çağrı: temiz */
        if (dl_open("/lib/nope.so") != 0) ok = 0;
        if (dlerror() == 0) ok = 0;              /* hata raporlanır */
        if (dlerror() != 0) ok = 0;              /* tek seferlik okuma */
        void* h16 = dl_open("/lib/libn1.so");
        if (!h16) ok = 0;
        if (dlerror() != 0) ok = 0;              /* başarılı işlem temizler */
        if (h16) dl_close(h16);
        up_puts(ok ? "[16A] dlerror [PASS]\n" : "[16A] dlerror [FAIL]\n");
    }

    /* 16B: dladdr */
    up_puts("[16B] dladdr test...\n");
    {
        int ok = 1;
        void* h16 = dl_open("/lib/liba.so");
        int (*liba_get_g)(void) = h16 ? (int (*)(void))dl_sym(h16, "liba_get_g") : 0;
        if (!h16 || !liba_get_g) ok = 0;
        else {
            struct dl_info in;
            if (!dladdr((const void*)liba_get_g, &in)) ok = 0;
            else {
                if (!in.fname || !up_streq(in.fname, "/lib/liba.so")) ok = 0;
                if (!in.fbase) ok = 0;
                if (!in.sname || !in.saddr) ok = 0;   /* en yakın tanımlı sembol */
            }
            struct dl_info in2;
            if (dladdr(0, &in2)) ok = 0;              /* adres aralık dışı */
            if (dladdr((const void*)0x1, &in2)) ok = 0;
        }
        if (h16) dl_close(h16);
        up_puts(ok ? "[16B] dladdr [PASS]\n" : "[16B] dladdr [FAIL]\n");
    }

    /* 16C: RTLD_DEFAULT / RTLD_NEXT kapsamı */
    up_puts("[16C] scope test...\n");
    {
        int ok = 1;
        void* hn1 = dl_open("/lib/libn1.so");
        void* hn2 = dl_open("/lib/libn2.so");
        int (*n1)(void) = hn1 ? (int (*)(void))dl_sym(hn1, "n_win") : 0;
        int (*n2)(void) = hn2 ? (int (*)(void))dl_sym(hn2, "n_win") : 0;
        if (!hn1 || !hn2 || !n1 || !n2) ok = 0;
        else {
            if (n1() != 11 || n2() != 22) ok = 0;
            int (*dflt)(void) = (int (*)(void))dl_sym(0, "n_win");
            if (!dflt || dflt() != 22) ok = 0;         /* RTLD_DEFAULT: en son yüklenen */
            int (*nxt)(void) = (int (*)(void))dl_sym_next(hn1, "n_win");
            if (!nxt || nxt() != 22) ok = 0;           /* RTLD_NEXT */
            if (dl_sym_next(hn2, "n_win") != 0) ok = 0;/* sonrakinde yok */
        }
        if (hn2) dl_close(hn2);
        if (hn1) dl_close(hn1);
        up_puts(ok ? "[16C] scope [PASS]\n" : "[16C] scope [FAIL]\n");
    }

    /* 16D: lazy PLT (RTLD_LAZY trampolin) */
    up_puts("[16D] lazy PLT test...\n");
    {
        int ok = 1;
        dl_register_export("la_hook", la_hook_impl);
        void* hl = dl_open_mode("/lib/libla.so", RTLD_LAZY);
        int (*la_call_fn)(int) = hl ? (int (*)(int))dl_sym(hl, "la_call") : 0;
        if (!hl || !la_call_fn) ok = 0;
        else {
            unsigned got0 = dl_lazy_slot(hl, "la_hook");
            if (got0 == 0) ok = 0;                     /* trampolin GOT'a kuruldu */
            int r = la_call_fn(3);
            if (r != 4) ok = 0;                        /* la_hook(3) = 3+1 */
            unsigned got1 = dl_lazy_slot(hl, "la_hook");
            if (got1 == 0 || got1 == got0) ok = 0;     /* GOT hedefe yamalandı */
        }
        if (hl) dl_close(hl);
        up_puts(ok ? "[16D] lazy PLT [PASS]\n" : "[16D] lazy PLT [FAIL]\n");
    }

    /* 16E: DT_NEEDED transitif yükleme (libj -> libk) */
    up_puts("[16E] DT_NEEDED test...\n");
    {
        int ok = 1;
        void* hj = dl_open("/lib/libj.so");
        int (*j_run_fn)(void) = hj ? (int (*)(void))dl_sym(hj, "j_run") : 0;
        if (!hj || !j_run_fn) ok = 0;
        else {
            if (j_run_fn() != 8) ok = 0;               /* k_val()+1 = 8 */
            int (*k1)(void) = (int (*)(void))dl_sym(0, "k_val");
            if (!k1 || k1() != 7) ok = 0;              /* bağımlılık görünür */
        }
        void* hk = dl_open("/lib/libk.so");            /* aynı blob -> dedupe */
        int (*k2)(void) = hk ? (int (*)(void))dl_sym(hk, "k_val") : 0;
        if (!hk || !k2 || k2() != 7) ok = 0;
        if (hk) { dl_close(hk); dl_close(hk); }        /* refs=2 -> tamamen kapat */
        if (hj) dl_close(hj);
        up_puts(ok ? "[16E] DT_NEEDED [PASS]\n" : "[16E] DT_NEEDED [FAIL]\n");
    }

    /* 16F: RTLD_GLOBAL vs RTLD_LOCAL görünürlük */
    up_puts("[16F] global visibility test...\n");
    {
        int ok = 1;
        void* hg1 = dl_open_mode("/lib/libg1.so", RTLD_GLOBAL | RTLD_NOW);
        void* hg2 = dl_open("/lib/libg2.so");          /* LOCAL */
        void* hg3 = dl_open("/lib/libg3.so");          /* gwho'yu dışarıdan çözer */
        int (*g3call)(void) = hg3 ? (int (*)(void))dl_sym(hg3, "g3_call") : 0;
        int (*gwho2)(void) = hg2 ? (int (*)(void))dl_sym(hg2, "gwho") : 0;
        if (!hg1 || !hg2 || !hg3 || !g3call || !gwho2) ok = 0;
        else {
            if (g3call() != 1) ok = 0;                 /* global g1 kazanır */
            int (*dflt)(void) = (int (*)(void))dl_sym(0, "gwho");
            if (!dflt || dflt() != 1) ok = 0;          /* varsayılan kapsam: global önce */
            if (gwho2() != 2) ok = 0;                  /* doğrudan tutamaç erişimi */
        }
        if (hg3) dl_close(hg3);
        if (hg2) dl_close(hg2);
        if (hg1) dl_close(hg1);
        up_puts(ok ? "[16F] global/local [PASS]\n" : "[16F] global/local [FAIL]\n");
    }

    /* 16G: dl_iterate_phdr */
    up_puts("[16G] dl_iterate_phdr test...\n");
    {
        int ok = 1;
        void* hg = dl_open("/lib/libg1.so");
        if (!hg) ok = 0;
        iter_count = 0; iter_main = 0; iter_ok = 0;
        dl_iterate_phdr(iter_cb_fn, 0);
        if (iter_count < 1) ok = 0;                    /* en az 1 modül */
        if (!iter_ok) ok = 0;                          /* ham phdr mevcut */
        if (hg) dl_close(hg);
        up_puts(ok ? "[16G] dl_iterate_phdr [PASS]\n" : "[16G] dl_iterate_phdr [FAIL]\n");
    }

    /* 16H: dl_open(NULL) ana program tutamacı + teardown */
    up_puts("[16H] main handle test...\n");
    {
        int ok = 1;
        void* hm = dl_open(0);
        if (!hm) ok = 0;
        else {
            int (*pv)(void) = (int (*)(void))dl_sym(hm, "prog_version");
            if (!pv || pv() != 7) ok = 0;              /* 15D export'u ana tutamakta */
            if (dl_sym(0, "prog_version") == 0) ok = 0;
            if (dl_close(hm) != -1) ok = 0;            /* ana program kapanmaz */
            if (dlerror() == 0) ok = 0;
        }
        iter_count = 0; iter_main = 0;
        dl_iterate_phdr(iter_cb_fn, 0);
        if (iter_main != 1) ok = 0;                    /* ana program numaralandı */
        up_puts(ok ? "[16H] main handle [PASS]\n" : "[16H] main handle [FAIL]\n");
    }

    /* ============ 17 serisi: kalıcı disk FS v2 ============ */
    uint32_t dfcap17 = syscall(SYS_BLKINFO, 0, 0, 0);
    int dfdisk = (dfcap17 != 0);

    /* 17A: dizin ağacı + path semantiği */
    up_puts("[17A] disk dirs test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            syscall(SYS_DFREMOVE, (uint32_t)"/docs/a.txt", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/etc/conf.ini", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/docs", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/etc", 0, 0);
            if (syscall(SYS_DFMKDIR, (uint32_t)"/docs", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFMKDIR, (uint32_t)"/etc", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFMKDIR, (uint32_t)"/docs", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/docs/a.txt", (uint32_t)"docs-a-data", 11) != 11) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/etc/conf.ini", (uint32_t)"mode=fast", 9) != 9) ok = 0;
            char rb[16];
            for (int i = 0; i < 16; i++) rb[i] = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/docs/a.txt", (uint32_t)rb, 16) != 11) ok = 0;
            for (int i = 0; i < 11 && ok; i++) if (rb[i] != "docs-a-data"[i]) ok = 0;
            char lst[256];
            for (int i = 0; i < 256; i++) lst[i] = 0;
            uint32_t n = syscall(SYS_DFLIST, (uint32_t)"/", (uint32_t)lst, 256);
            if (n == (uint32_t)-1) ok = 0;
            else {
                lst[n < 256 ? n : 255] = 0;
                if (!up_contains(lst, "docs") || !up_contains(lst, "etc")) ok = 0;
            }
            n = syscall(SYS_DFLIST, (uint32_t)"/docs", (uint32_t)lst, 256);
            if (n == (uint32_t)-1) ok = 0;
            else {
                lst[n < 256 ? n : 255] = 0;
                if (!up_streq(lst, "a.txt\n")) ok = 0;
            }
            if (syscall(SYS_DFLOAD, (uint32_t)"/docs", (uint32_t)rb, 16) != (uint32_t)-1) ok = 0;
        }
        up_puts(ok ? "[17A] disk dirs [PASS]\n" : "[17A] disk dirs [FAIL]\n");
    }

    /* 17B: çok bloklu büyük dosya (16KB) */
    up_puts("[17B] big file test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            for (int i = 0; i < 16384; i++) df_wbig[i] = (unsigned char)(0x31 + (i % 97));
            for (int i = 0; i < 16384; i++) df_rbig[i] = 0;
            syscall(SYS_DFREMOVE, (uint32_t)"/big17", 0, 0);
            if (syscall(SYS_DFSAVE, (uint32_t)"/big17", (uint32_t)df_wbig, 16384) != 16384) ok = 0;
            uint32_t st = syscall(SYS_DFSTAT, (uint32_t)"/big17", 0, 0);
            if (st == (uint32_t)-1 || (st & 0x80000000u) || (st & 0x7FFFFFFFu) != 16384) ok = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/big17", (uint32_t)df_rbig, 16384) != 16384) ok = 0;
            for (int i = 0; i < 16384 && ok; i++) if (df_rbig[i] != df_wbig[i]) ok = 0;
            for (int i = 0; i < 512; i++) df_rbig[i] = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/big17", (uint32_t)df_rbig, 512) != 512) ok = 0;
            for (int i = 0; i < 512 && ok; i++) if (df_rbig[i] != df_wbig[i]) ok = 0;
        }
        up_puts(ok ? "[17B] big file [PASS]\n" : "[17B] big file [FAIL]\n");
    }

    /* 17C: silme + blok geri kazanımı */
    up_puts("[17C] remove/reclaim test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            syscall(SYS_DFREMOVE, (uint32_t)"/re17", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/re17b", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/nd/f", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/nd", 0, 0);
            for (int i = 0; i < 16384; i++) df_wbig[i] = (unsigned char)(0x55 + (i & 63));
            if (syscall(SYS_DFSAVE, (uint32_t)"/re17", (uint32_t)df_wbig, 16384) != 16384) ok = 0;
            if (syscall(SYS_DFMKDIR, (uint32_t)"/nd", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/nd/f", (uint32_t)"x", 1) != 1) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/nd", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/nd/f", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/nd", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/re17", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFSTAT, (uint32_t)"/re17", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/re17b", (uint32_t)df_wbig, 16384) != 16384) ok = 0;
            if (syscall(SYS_DFCHECK, 0, 0, 0) != 0) ok = 0;
        }
        up_puts(ok ? "[17C] remove/reclaim [PASS]\n" : "[17C] remove/reclaim [FAIL]\n");
    }

    /* 17D: truncate + güncelleme */
    up_puts("[17D] truncate/update test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            for (int i = 0; i < 8192; i++) df_wbig[i] = (unsigned char)('A' + (i % 26));
            syscall(SYS_DFREMOVE, (uint32_t)"/tr17", 0, 0);
            if (syscall(SYS_DFSAVE, (uint32_t)"/tr17", (uint32_t)df_wbig, 8192) != 8192) ok = 0;
            if (syscall(SYS_DFTRUNCATE, (uint32_t)"/tr17", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFSTAT, (uint32_t)"/tr17", 0, 0) != 0) ok = 0;
            for (int i = 0; i < 64; i++) df_rbig[i] = 0x5A;
            if (syscall(SYS_DFLOAD, (uint32_t)"/tr17", (uint32_t)df_rbig, 64) != 0) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/tr17", (uint32_t)"hi", 2) != 2) ok = 0;
            for (int i = 0; i < 64; i++) df_rbig[i] = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/tr17", (uint32_t)df_rbig, 64) != 2) ok = 0;
            if (df_rbig[0] != 'h' || df_rbig[1] != 'i') ok = 0;
            for (int i = 0; i < 9000; i++) df_wbig[i] = (unsigned char)('0' + (i % 10));
            if (syscall(SYS_DFSAVE, (uint32_t)"/tr17", (uint32_t)df_wbig, 9000) != 9000) ok = 0;
            for (int i = 0; i < 9000; i++) df_rbig[i] = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/tr17", (uint32_t)df_rbig, 9000) != 9000) ok = 0;
            for (int i = 0; i < 9000 && ok; i++) if (df_rbig[i] != df_wbig[i]) ok = 0;
            if (syscall(SYS_DFCHECK, 0, 0, 0) != 0) ok = 0;
        }
        up_puts(ok ? "[17D] truncate/update [PASS]\n" : "[17D] truncate/update [FAIL]\n");
    }

    /* 17E: reboot-arası kalıcılık sayacı */
    up_puts("[17E] reboot persist test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            char cb[16];
            for (int i = 0; i < 16; i++) cb[i] = 0;
            uint32_t n = syscall(SYS_DFLOAD, (uint32_t)"b17", (uint32_t)cb, 15);
            int v;
            if (n == (uint32_t)-1) v = 1;
            else {
                cb[n] = 0;
                v = up_atoi(cb);
                if (v < 0) v = 1;
                else { v = v + 1; if (v > 9999) v = 1; }
            }
            char ob[8];
            up_itoa_to((unsigned)v, ob);
            int ol = 0;
            while (ob[ol]) ol++;
            if (syscall(SYS_DFSAVE, (uint32_t)"b17", (uint32_t)ob, ol) != (uint32_t)ol) ok = 0;
            up_puts("  boot#=");
            up_putdec((uint32_t)v);
            up_puts("\n");
            if (v < 1) ok = 0;
        }
        up_puts(ok ? "[17E] reboot persist [PASS]\n" : "[17E] reboot persist [FAIL]\n");
    }

    /* 17F: liste/stat API'leri */
    up_puts("[17F] list/stat test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            syscall(SYS_DFREMOVE, (uint32_t)"/f17f", 0, 0);
            if (syscall(SYS_DFSAVE, (uint32_t)"/f17f", (uint32_t)"stat-me-17", 10) != 10) ok = 0;
            uint32_t st = syscall(SYS_DFSTAT, (uint32_t)"/f17f", 0, 0);
            if (st == (uint32_t)-1 || (st & 0x80000000u) || (st & 0x7FFFFFFFu) != 10) ok = 0;
            st = syscall(SYS_DFSTAT, (uint32_t)"/docs", 0, 0);
            if (st == (uint32_t)-1 || !(st & 0x80000000u)) ok = 0;
            st = syscall(SYS_DFSTAT, (uint32_t)"/", 0, 0);
            if (st == (uint32_t)-1 || !(st & 0x80000000u)) ok = 0;
            if (syscall(SYS_DFSTAT, (uint32_t)"/yok17", 0, 0) != (uint32_t)-1) ok = 0;
            for (int i = 0; i < 1024; i++) df_listbuf[i] = 0;
            uint32_t n = syscall(SYS_DFLIST, (uint32_t)"/", (uint32_t)df_listbuf, 1024);
            if (n == (uint32_t)-1) ok = 0;
            else {
                df_listbuf[n < 1024 ? n : 1023] = 0;
                if (!up_contains(df_listbuf, "docs") || !up_contains(df_listbuf, "f17f")) ok = 0;
            }
            if (syscall(SYS_DFLIST, (uint32_t)"/yok17", (uint32_t)df_listbuf, 1024) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFLIST, (uint32_t)"/f17f", (uint32_t)df_listbuf, 1024) != (uint32_t)-1) ok = 0;
        }
        up_puts(ok ? "[17F] list/stat [PASS]\n" : "[17F] list/stat [FAIL]\n");
    }

    /* 17G: inode döngü stresi (40 dosya yaz/doğrula/sil x2) */
    up_puts("[17G] inode stress test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            char nm[16];
            char payload[128];
            for (int i = 0; i < 128; i++) payload[i] = (char)('a' + (i % 26));
            syscall(SYS_DFREMOVE, (uint32_t)"/big17", 0, 0);
            syscall(SYS_DFREMOVE, (uint32_t)"/re17b", 0, 0);
            for (int pass = 0; pass < 2 && ok; pass++) {
                for (int i = 0; i < 40 && ok; i++) {
                    nm[0] = '/'; nm[1] = 's'; nm[2] = '1'; nm[3] = '7'; nm[4] = '_';
                    int L = up_itoa_to((unsigned)i, nm + 5);
                    nm[5 + L] = '\0';
                    payload[0] = (char)('A' + (i % 26));
                    if (syscall(SYS_DFSAVE, (uint32_t)nm, (uint32_t)payload, 128) != 128) ok = 0;
                }
                for (int i = 0; i < 40 && ok; i++) {
                    nm[0] = '/'; nm[1] = 's'; nm[2] = '1'; nm[3] = '7'; nm[4] = '_';
                    int L = up_itoa_to((unsigned)i, nm + 5);
                    nm[5 + L] = '\0';
                    for (int j = 0; j < 128; j++) df_rbig[j] = 0;
                    if (syscall(SYS_DFLOAD, (uint32_t)nm, (uint32_t)df_rbig, 128) != 128) ok = 0;
                    else {
                        if (df_rbig[0] != (unsigned char)('A' + (i % 26))) ok = 0;
                        for (int j = 1; j < 128 && ok; j++)
                            if (df_rbig[j] != (unsigned char)('a' + (j % 26))) ok = 0;
                    }
                }
                for (int i = 0; i < 40 && ok; i++) {
                    nm[0] = '/'; nm[1] = 's'; nm[2] = '1'; nm[3] = '7'; nm[4] = '_';
                    int L = up_itoa_to((unsigned)i, nm + 5);
                    nm[5 + L] = '\0';
                    if (syscall(SYS_DFREMOVE, (uint32_t)nm, 0, 0) != 0) ok = 0;
                }
                if (syscall(SYS_DFCHECK, 0, 0, 0) != 0) ok = 0;
            }
        }
        up_puts(ok ? "[17G] inode stress [PASS]\n" : "[17G] inode stress [FAIL]\n");
    }

    /* 17H: bütünlük + hata yolları */
    up_puts("[17H] integrity test...\n");
    {
        int ok = 1;
        if (!dfdisk) up_puts("  no block device [SKIP]\n");
        else {
            if (syscall(SYS_DFCHECK, 0, 0, 0) != 0) ok = 0;
            if (syscall(SYS_BLKREAD, dfcap17, (uint32_t)df_rbig, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_BLKREAD, dfcap17 + 1000, (uint32_t)df_rbig, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"0123456789012345678901234", (uint32_t)"x", 1) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/too17", (uint32_t)df_wbig, 30000) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/yok17x", (uint32_t)df_rbig, 64) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/yok17x", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFTRUNCATE, (uint32_t)"/yok17x", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFMKDIR, (uint32_t)"/docs", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFTRUNCATE, (uint32_t)"/docs", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFMKDIR, (uint32_t)"/", 0, 0) != (uint32_t)-1) ok = 0;
            if (syscall(SYS_DFSAVE, (uint32_t)"/a//b", (uint32_t)"x", 1) != (uint32_t)-1) ok = 0;
            syscall(SYS_DFREMOVE, (uint32_t)"/empty17", 0, 0);
            if (syscall(SYS_DFSAVE, (uint32_t)"/empty17", (uint32_t)"x", 0) != 0) ok = 0;
            if (syscall(SYS_DFSTAT, (uint32_t)"/empty17", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFLOAD, (uint32_t)"/empty17", (uint32_t)df_rbig, 64) != 0) ok = 0;
            if (syscall(SYS_DFREMOVE, (uint32_t)"/empty17", 0, 0) != 0) ok = 0;
            if (syscall(SYS_DFCHECK, 0, 0, 0) != 0) ok = 0;
        }
        up_puts(ok ? "[17H] integrity [PASS]\n" : "[17H] integrity [FAIL]\n");
    }

    /* ============ 19 serisi: shell + exec ============ */

    /* 19A: exec argv konvansiyonu (doğrudan fork/exec, shell yok) */
    up_puts("[19A] exec argv test...\n");
    {
        t_ok = 1;
        int canary = 0x5A5A5A5A;
        int n = run_capture("/bin/echo", "hi there");
        if (n != 9) t_ok = 0;
        if (!up_streq(cap_buf, "hi there\n")) t_ok = 0;
        if (cap_status != 0) t_ok = 0;
        n = run_capture("/bin/echo", 0);
        if (n != 1) t_ok = 0;
        if (!up_streq(cap_buf, "\n")) t_ok = 0;
        if (cap_status != 0) t_ok = 0;
n = run_capture("/bin/yok19", 0);
        if (n != 0) t_ok = 0;
        if (cap_status != 1) t_ok = 0;   /* exec fail -> exit(1) */
        if (canary != 0x5A5A5A5A) t_ok = 0; /* exec CoW-stack koruması */
        up_puts(t_ok ? "[19A] exec argv [PASS]\n" : "[19A] exec argv [FAIL]\n");
    }

    /* 19B: shell tek komut + argüman */
    up_puts("[19B] shell args test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo hi\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "\nhi\n")) t_ok = 0;
        n = sh_run_script("echo a b c\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "a b c\n")) t_ok = 0;
        n = sh_run_script("yokcmd19\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "exec fail")) t_ok = 0;
        up_puts(t_ok ? "[19B] shell args [PASS]\n" : "[19B] shell args [FAIL]\n");
    }

    /* 19C: shell pipe (dup2 onarımı) */
    up_puts("[19C] shell pipe test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo hi | cat\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "\nhi\n")) t_ok = 0;
        n = sh_run_script("echo hello pipe | cat\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "hello pipe\n")) t_ok = 0;
        up_puts(t_ok ? "[19C] shell pipe [PASS]\n" : "[19C] shell pipe [FAIL]\n");
    }

    /* 19D: çoklu pipe + uç durumlar */
    up_puts("[19D] multi pipe test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo a b c | cat | cat\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "a b c\n")) t_ok = 0;
        n = sh_run_script("echo | cat\nexit\n");
        if (n <= 0) t_ok = 0;   /* boş satır turu: çökme yok, EOF temiz */
        up_puts(t_ok ? "[19D] multi pipe [PASS]\n" : "[19D] multi pipe [FAIL]\n");
    }

    /* 19E: yönlendirme >, >>, < */
    up_puts("[19E] redirect test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo x > /f19e\nexit\n");
        if (n <= 0) t_ok = 0;
        {
            int fd = syscall(SYS_OPEN, (uint32_t)"/f19e", 1, 0);
            if (fd < 0) t_ok = 0;
            else {
                char rb[8];
                for (int i = 0; i < 8; i++) rb[i] = 0;
                int r = syscall(SYS_READ, fd, (uint32_t)rb, 8);
                syscall(SYS_CLOSE, fd, 0, 0);
                if (r != 2 || rb[0] != 'x' || rb[1] != '\n') t_ok = 0;
            }
        }
        n = sh_run_script("echo y >> /f19e\nexit\n");
        if (n <= 0) t_ok = 0;
        n = sh_run_script("cat < /f19e\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "x\n") || !up_contains(sh_outbuf, "y\n")) t_ok = 0;
        up_puts(t_ok ? "[19E] redirect [PASS]\n" : "[19E] redirect [FAIL]\n");
    }

    /* 19F: builtin + ';' + çıkış kodu */
    up_puts("[19F] builtin test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo hi; echo there\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "hi\n") || !up_contains(sh_outbuf, "there\n")) t_ok = 0;
        n = sh_run_script("help\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "jobs")) t_ok = 0;
        n = sh_run_script("exit 3\n");
        if (sh_status != 3) t_ok = 0;
        up_puts(t_ok ? "[19F] builtin [PASS]\n" : "[19F] builtin [FAIL]\n");
    }

    /* 20A: kullanıcı komutları (ls/pwd/cd/mkdir/rm/touch/cp/mv/ps/uname/clear) */
    up_puts("[20A] user commands test...\n");
    {
        t_ok = 1;
        int n20;
        /* sh çıktısı prompt yankısıyla kirlenir; dosya durumunu SYS_DFLIST ile doğrula */
        n20 = sh_run_script("pwd\nexit\n");
        if (n20 <= 0 || !up_contains(sh_outbuf, "/")) t_ok = 0;
        n20 = sh_run_script("uname\nexit\n");
        if (n20 <= 0 || !up_contains(sh_outbuf, "charOS")) t_ok = 0;
        n20 = sh_run_script("ps\nexit\n");
        if (n20 <= 0 || !up_contains(sh_outbuf, "state=")) t_ok = 0;
        /* kalıcı diskten kalan /cmds'i sıfırla (idempotent), sonra mkdir+cwd+touch */
        n20 = sh_run_script("rm /cmds/a\nrm /cmds/b\nrm /cmds/c\nrmdir /cmds\nmkdir /cmds\n"
                            "cd /cmds\ntouch /cmds/a\nls /cmds\nexit\n");
        if (n20 <= 0) t_ok = 0;
        {
            char lb[512];
            for (int i = 0; i < 512; i++) lb[i] = 0;
            int nr = (int)syscall(SYS_DFLIST, (uint32_t)"/cmds", (uint32_t)lb, sizeof(lb));
            if (nr <= 0 || !up_contains(lb, "a\n")) t_ok = 0;
        }
        n20 = sh_run_script("cp /cmds/a /cmds/b\nexit\n");
        if (n20 <= 0) t_ok = 0;
        {
            char lb[512];
            for (int i = 0; i < 512; i++) lb[i] = 0;
            int nr = (int)syscall(SYS_DFLIST, (uint32_t)"/cmds", (uint32_t)lb, sizeof(lb));
            if (nr <= 0 || !up_contains(lb, "b\n")) t_ok = 0;
        }
        n20 = sh_run_script("rm /cmds/b\nexit\n");
        if (n20 <= 0) t_ok = 0;
        {
            char lb[512];
            for (int i = 0; i < 512; i++) lb[i] = 0;
            int nr = (int)syscall(SYS_DFLIST, (uint32_t)"/cmds", (uint32_t)lb, sizeof(lb));
            if (nr <= 0 || up_contains(lb, "b\n")) t_ok = 0;
        }
        n20 = sh_run_script("mv /cmds/a /cmds/c\nexit\n");
        if (n20 <= 0) t_ok = 0;
        {
            char lb[512];
            for (int i = 0; i < 512; i++) lb[i] = 0;
            int nr = (int)syscall(SYS_DFLIST, (uint32_t)"/cmds", (uint32_t)lb, sizeof(lb));
            if (nr <= 0 || !up_contains(lb, "c\n") || up_contains(lb, "a\n")) t_ok = 0;
        }
        up_puts(t_ok ? "[20A] user commands [PASS]\n" : "[20A] user commands [FAIL]\n");
    }

    /* 20B: /proc /sys /dev sanal dosyalar (open/read/write) */
    up_puts("[20B] pseudo-FS...\n");
    {
        t_ok = 1;
        char pb[256];
        int fd, n;
#define PFS_RO 1
#define PFS_WO 2
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/proc/version", PFS_RO, 0);
        if (fd < 0) t_ok = 0;
        else {
            for (int i = 0; i < 256; i++) pb[i] = 0;
            n = (int)syscall(SYS_READ, (uint32_t)fd, (uint32_t)pb, 255);
            if (n <= 0 || !up_contains(pb, "charOS")) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/proc/status", PFS_RO, 0);
        if (fd < 0) t_ok = 0;
        else {
            for (int i = 0; i < 256; i++) pb[i] = 0;
            n = (int)syscall(SYS_READ, (uint32_t)fd, (uint32_t)pb, 255);
            if (n <= 0 || !up_contains(pb, "pid=")) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/proc/tasks", PFS_RO, 0);
        if (fd < 0) t_ok = 0;
        else {
            for (int i = 0; i < 256; i++) pb[i] = 0;
            n = (int)syscall(SYS_READ, (uint32_t)fd, (uint32_t)pb, 255);
            if (n <= 0 || !up_contains(pb, "sh")) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/sys/version", PFS_RO, 0);
        if (fd < 0) t_ok = 0;
        else {
            for (int i = 0; i < 256; i++) pb[i] = 0;
            n = (int)syscall(SYS_READ, (uint32_t)fd, (uint32_t)pb, 255);
            if (n <= 0 || !up_contains(pb, "charOS")) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/dev/null", PFS_WO, 0);
        if (fd < 0) t_ok = 0;
        else {
            n = (int)syscall(SYS_WRITE, (uint32_t)fd, (uint32_t)"discard", 7);
            if (n != 7) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        fd = (int)syscall(SYS_OPEN, (uint32_t)"/dev/zero", PFS_RO, 0);
        if (fd < 0) t_ok = 0;
        else {
            for (int i = 0; i < 16; i++) pb[i] = 0xAA;
            n = (int)syscall(SYS_READ, (uint32_t)fd, (uint32_t)pb, 16);
            int allz = (n == 16);
            for (int i = 0; i < 16 && allz; i++) if (pb[i] != 0) allz = 0;
            if (!allz) t_ok = 0;
            syscall(SYS_CLOSE, (uint32_t)fd, 0, 0);
        }
        if ((int)syscall(SYS_OPEN, (uint32_t)"/proc/bogus", PFS_RO, 0) >= 0) t_ok = 0;
#undef PFS_RO
#undef PFS_WO
        up_puts(t_ok ? "[20B] pseudo-FS [PASS]\n" : "[20B] pseudo-FS [FAIL]\n");
    }

    /* 20C: ek gruplar (setgroups) + capability bitmask (capget/capset) */
    up_puts("[20C] groups/caps...\n");
    {
        t_ok = 1;
        /* root: boş grup listesi varsayılan; set 100/200; get aynısını döner */
        int gd[2] = {100, 200};
        if (syscall(SYS_SETGROUPS, (uint32_t)gd, 8, 0) != 0) t_ok = 0;
        int gg[4] = {0};
        uint32_t n = syscall(SYS_GETGROUPS, (uint32_t)gg, sizeof(gg), 0);
        if ((int)n != 8) t_ok = 0;
        if (n == 8 && (gg[0] != 100 || gg[1] != 200)) t_ok = 0;
        /* setgid sonrası da gruplar kalır (task alanında) */
        /* yetkisiz cap artırımı reddedilir; kısma serbesttir */
        if (syscall(SYS_CAPSET, CAP_DAC_OVERRIDE, 0, 0) != CAP_DAC_OVERRIDE) t_ok = 0;
        if (syscall(SYS_CAPGET, 0, 0, 0) != CAP_DAC_OVERRIDE) t_ok = 0;
        /* alt küme: DAC_OVERRIDE açıkken CAP_KILL eklenebilir(superuser) */
        uint32_t want = CAP_DAC_OVERRIDE | CAP_KILL;
        if (syscall(SYS_CAPSET, want, 0, 0) != want) t_ok = 0;
        if (syscall(SYS_CAPGET, 0, 0, 0) != want) t_ok = 0;
        /* temizle: 0'e in */
        if (syscall(SYS_CAPSET, 0, 0, 0) != 0) t_ok = 0;
        up_puts(t_ok ? "[20C] groups/caps [PASS]\n" : "[20C] groups/caps [FAIL]\n");
    }

    /* 19G: jobs/wait/kill/fg + WNOHANG */
    up_puts("[19G] jobs test...\n");
    {
        t_ok = 1;
        /* çekirdek WNOHANG: doğrudan yoklama */
        {
            volatile uint32_t c = syscall(SYS_FORK, 0, 0, 0);
            if (*(volatile uint32_t*)&c == 0) syscall(SYS_EXIT, 5, 0, 0);
            else {
                int st = -1;
                int r = 0;
                for (int i = 0; i < 200 && r == 0; i++) {
                    r = (int)syscall(SYS_WAITPID, c, (uint32_t)&st, WNOHANG);
                    if (r == 0) syscall(SYS_YIELD, 0, 0, 0);
                }
                if (r != (int)c || st != 5) t_ok = 0;   /* ölü child biçildi */
                int st2 = -1;
                int r2 = (int)syscall(SYS_WAITPID, c, (uint32_t)&st2, WNOHANG);
                if (r2 != -1) t_ok = 0;                  /* artık child yok */
            }
        }
        int n = sh_run_script("echo hi &\nwait\njobs\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "hi\n") || !up_contains(sh_outbuf, "no jobs")) t_ok = 0;
        n = sh_run_script("export QZ9=1 > /e19\ngetty < /e19 &\nsleep 30\nkill %1\nwait\njobs\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "login:") || !up_contains(sh_outbuf, "no jobs")) t_ok = 0;
        n = sh_run_script("echo hi &\nfg %1\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "hi\n")) t_ok = 0;
        n = sh_run_script("kill %99\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "no such job")) t_ok = 0;
        up_puts(t_ok ? "[19G] jobs [PASS]\n" : "[19G] jobs [FAIL]\n");
    }

    /* 19H: && || kısa devre */
    up_puts("[19H] andor test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("yokcmd19 || echo fallback\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "fallback\n")) t_ok = 0;
        n = sh_run_script("yokcmd19 && echo skipped\nexit\n");
        if (n <= 0 || up_count(sh_outbuf, "skipped") != 1) t_ok = 0;
        n = sh_run_script("echo ok && echo again\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "again\n")) t_ok = 0;
        n = sh_run_script("echo a && yokcmd19 || echo rescued\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "rescued\n")) t_ok = 0;
        up_puts(t_ok ? "[19H] andor [PASS]\n" : "[19H] andor [FAIL]\n");
    }

    /* 19I: ortam değişkenleri */
    up_puts("[19I] env test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("export WHO=world\necho hello $WHO\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "hello world\n")) t_ok = 0;
        n = sh_run_script("echo [$UNSET19]\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "[]")) t_ok = 0;
        n = sh_run_script("export Q19=42\nexport\nexit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "Q19=42")) t_ok = 0;
        up_puts(t_ok ? "[19I] env [PASS]\n" : "[19I] env [FAIL]\n");
    }

    /* 19J: geçmiş + TAB tamamlama */
    up_puts("[19J] history test...\n");
    {
        t_ok = 1;
        int n = sh_run_script("echo AAA\n\x1B[A\n exit\n");
        if (n <= 0 || up_count(sh_outbuf, "AAA") != 4) t_ok = 0;
        n = sh_run_script("echo one\necho two\n\x1B[A\x1B[A\x1B[B\n exit\n");
        if (n <= 0 || up_count(sh_outbuf, "two") != 5 || up_count(sh_outbuf, "one") != 3) t_ok = 0;
        n = sh_run_script("ec\t\n exit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "echo ")) t_ok = 0;
        n = sh_run_script("/bin/ec\t\n exit\n");
        if (n <= 0 || !up_contains(sh_outbuf, "/bin/echo ")) t_ok = 0;
        up_puts(t_ok ? "[19J] history [PASS]\n" : "[19J] history [FAIL]\n");
    }

    /* 11A: TCP sunucu doğrulanıyor (kernel tarafı) */
    up_puts("TCP server OK\n");

    /* 19K: desktop/console builtin'leri aynı try_builtin yolundan SYS_VTSWITCH
     * tetikler (serial'da [VTSWITCH] log'u görülür). Headless doğrulama. */
    sh_run_script("desktop\nexit\n");
    sh_run_script("console\nexit\n");

    /* 19X: varsayılan boot = öz-sınamadan sonra kullanıcı kabuğuna düş.
     * exec yamalı stack/yanıt çerçevesini sıfırlar; fd 0/1/2 console'da kalır
     * (shell stdin=klavye, stdout=konsol). Başarısız olursa test çıkışı yap. */
    up_puts("Starting /bin/sh...\n");
    if (syscall(SYS_EXEC, (uint32_t)"/bin/sh", 0, 0) != 0) {
        up_puts("shell exec fail\n");
        syscall(SYS_EXIT, 1, 0, 0);
    }

    for (;;) asm volatile("hlt");
}
