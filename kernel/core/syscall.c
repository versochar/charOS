#include <core/syscall.h>
#include <core/idt.h>
#include <core/isr.h>
#include <process/task.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <memory/paging.h>
#include <fs/chfs.h>
#include <fs/diskfs.h>
#include <drivers/blk.h>
#include <process/pipe.h>
#include <process/fork.h>
#include <process/cap.h>
#include <process/sandbox.h>
#include <core/doc.h>
#include <core/abi.h>
#include <core/auth.h>
#include <drivers/input.h>
#include <drivers/vt.h>
#include <string.h>

/* User stack için dışarıdan gelen - 6B'de kullanılacak */
extern void jump_to_user_mode(uint32_t entry, uint32_t stack);

/* Pipe için global buffer (9A) */
static struct pipe_buf global_pipe __attribute__((used));

/* 11C-1: fs_read/fs_write bounce buffer - kullanıcı buffer'ını doğrudan kernel'a
   memcpy'lemek yerine, sınırlı ve daima map edilmiş kernel buffer'ına kopyala.
   Bu, kullanıcı buffer'ının sayfa sınırını aşıp sayfa map yokken page fault
   atmasını veya heap'i bozmasını engeller. */
#define BOUNCE_SIZE 32768
static uint8_t bounce_buf[BOUNCE_SIZE];

/* Syscall tablosu */
typedef uint32_t (*syscall_fn_t)(uint32_t, uint32_t, uint32_t);
static syscall_fn_t syscall_table[256] = {0};

/* int 0x80 handler stub - assembly */
extern void syscall_stub(void);

static int is_user_buf_valid(uint32_t addr, uint32_t len) {
    if (!addr || len > 4096) return 0;
    if (addr < 0x400000) return 0; // kernel altı erişim engelle
    /* Buffer boyunca her sayfa map edilmiş olmalı - tek ilk-sayfa kontrolü yetmez */
    uint32_t end = addr + len;
    for (uint32_t a = addr; a < end; a += 0x1000) {
        if (!paging_get_phys(a)) return 0;
    }
    if (!paging_get_phys(end - 1)) return 0; // son byte da map edilmiş olmalı
    return 1;
}

/* 17B: büyük disk transferleri için bounce-sınırlı doğrulama (32KB).
 * copy_* ve dfsave/dfload/dflist kullanır; diğer syscall'lar kendi
 * 4KB sınırlarını çağrı başında uygular. */
static int is_user_buf_valid_big(uint32_t addr, uint32_t len) {
    if (!addr || len > BOUNCE_SIZE) return 0;
    if (addr < 0x400000) return 0;
    uint32_t end = addr + len;
    for (uint32_t a = addr; a < end; a += 0x1000) {
        if (!paging_get_phys(a)) return 0;
    }
    if (!paging_get_phys(end - 1)) return 0;
    return 1;
}

/* 11C-1: User'dan kernel bounce buffer'a güvenli kopyalama (sayfa sayfa doğrula) */
static int copy_from_user(uint32_t user_ptr, void* dst, uint32_t len) {
    if (!is_user_buf_valid_big(user_ptr, len)) return -1;
    /* Kernel, user virtaul adreslerine doğrudan erişebilir (aynı page dir) ama
       sınırlı ve kontrol altında kopyalarız. */
    memcpy(dst, (const void*)user_ptr, len);
    return 0;
}

/* 11C-1: Kernel buffer'dan user adrese güvenli kopyalama */
static int copy_to_user(uint32_t user_ptr, const void* src, uint32_t len) {
    if (!is_user_buf_valid_big(user_ptr, len)) return -1;
    memcpy((void*)user_ptr, src, len);
    return 0;
}

/* 17: diskfs yolunu user'dan kernel'a kopyala (25 baytlık kname'e). Dönüş: uzunluk / -1 */
static int df_copy_path(uint32_t uname, char* kname) {
    if (!uname || !paging_get_phys(uname)) return -1;
    int nl = 0;
    const char* up = (const char*)uname;
    while (nl < 24) {
        if (!paging_get_phys(uname + nl)) return -1;
        if (up[nl] == '\0') break;
        nl++;
    }
    if (nl >= 24) return -1;
    if (copy_from_user(uname, kname, nl + 1) != 0) return -1;
    kname[nl] = '\0';
    return nl;
}

/* 12B: Aktif syscall trap frame - sys_fork buradan child state'i kopyalar */
static struct registers* syscall_cur_regs = 0;
struct registers* syscall_get_regs(void) { return syscall_cur_regs; }

/* İlk implementasyonlar */
static uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t count) {
    if (count > 4096) count = 4096; // bounce buffer sınırı
    /* 19C: önce fd tablosu — dup2 ile pipe/dosyaya yönlendirilmiş 1/2
     * tablo yolundan gider (sys_read'deki fd-0 deseni). Console sadeece
     * hâlâ FD_CONSOLE olan 1/2'ye düşer. */
    struct task* _wc = task_current();
    int wredir = (_wc && fd < 16 && _wc->fd_table[fd].valid &&
        (_wc->fd_table[fd].type == FD_PIPE || _wc->fd_table[fd].type == FD_FILE));
    if (!wredir && (fd == 1 || fd == 2)) { // stdout/stderr
        if (!is_user_buf_valid(buf, count)) {
            serial_puts("[write] invalid buf=0x"); serial_puthex(buf); serial_puts(" count="); serial_puthex(count); serial_puts(" pid="); serial_puthex(task_current()->pid); serial_puts("\n");
            return (uint32_t)-1;
        }
        const char* str = (const char*)buf;
        for (uint32_t i = 0; i < count && str[i]; i++) {
            vga_putc(str[i]);
            serial_putc(str[i]);
        }
        /* 19K: konsol modunda (text VT aktif) framebuffer'a satır olarak bas —
         * userland sh çıktısı ekranda görünsün. Tek render (performans). */
        if (vt_active() != VT_GUI &&
            copy_from_user(buf, bounce_buf, count) == 0)
            vt_write_n(vt_active(), (const char*)bounce_buf, (int)count);
        return count;
    }
    // 12D: pipe ise blocking write
    struct task* cur = task_current();
    if (cur && fd < 16 && cur->fd_table[fd].valid && cur->fd_table[fd].type == FD_PIPE) {
        if (!is_user_buf_valid(buf, count)) return (uint32_t)-1;
        if (copy_from_user(buf, bounce_buf, count) != 0) return (uint32_t)-1;
        struct pipe* p = cur->fd_table[fd].pipe;
        if (!p || !cur->fd_table[fd].pipe_write) return (uint32_t)-1; // read end'e yazılamaz
        return (uint32_t)pipe_write_block(p, (const char*)bounce_buf, (int)count);
    }
    // File descriptor yazma: kullanıcı buffer'ını kernel bounce buffer'a kopyala,
    // fs_write yalnızca daima-map'lı bounce buffer ile çalışsın (heap koruması).
    if (copy_from_user(buf, bounce_buf, count) != 0) return (uint32_t)-1;
    return (uint32_t)fs_write((int)fd, (const char*)bounce_buf, (int)count);
}

static uint32_t sys_read(uint32_t fd, uint32_t buf, uint32_t count) {
    if (count > 4096) count = 4096; // bounce buffer sınırı
    // 12D: fd 0 sorgusuz klavye DEĞİL - önce tabloya bak; valid FILE/PIPE ise
    // o yol işler (19B: pipe duplı stdin klavyeye düşmemeli). OPEN artık
    // 0/1/2 vermez ama fork öncesi açılanlar/dup2 ile taşınanlar olabilir.
    if (fd == 0) {
        struct task* _rc = task_current();
        if (_rc && fd < 16 && _rc->fd_table[fd].valid &&
            (_rc->fd_table[fd].type == FD_FILE || _rc->fd_table[fd].type == FD_PIPE)) {
            // aşağıda genel file/pipe yolu işler
        } else {
            if (!is_user_buf_valid(buf, count)) return (uint32_t)-1;
            char* dest = (char*)buf;
            uint32_t i = 0;
            while (i < count) {
                char c = keyboard_getchar();
                dest[i++] = c;
                if (c == '\n') break;
            }
            return i;
        }
    }
    // 12D: pipe ise blocking read
    struct task* cur = task_current();
    if (cur && fd < 16 && cur->fd_table[fd].valid && cur->fd_table[fd].type == FD_PIPE) {
        if (!is_user_buf_valid(buf, count)) return (uint32_t)-1;
        struct pipe* p = cur->fd_table[fd].pipe;
        if (!p || cur->fd_table[fd].pipe_write) return (uint32_t)-1; // write end'den okunamaz
        int n = pipe_read_block(p, (char*)bounce_buf, (int)count);
        if (n <= 0) return (uint32_t)n;
        if (copy_to_user(buf, bounce_buf, (uint32_t)n) != 0) return (uint32_t)-1;
        return (uint32_t)n;
    }
    // File descriptor'dan okuma: fs_read kernel bounce buffer'a yazsın, sonra
    // kullanıcı adresine güvenli kopyala.
    int n = fs_read((int)fd, (char*)bounce_buf, (int)count);
    if (n <= 0) return (uint32_t)n;
    if (copy_to_user(buf, bounce_buf, (uint32_t)n) != 0) return (uint32_t)-1;
    return (uint32_t)n;
}

static uint32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    struct task* cur = task_current();
    if (!cur) return 0;
    return (uint32_t)cur->pid;
}

static uint32_t sys_exit(uint32_t status, uint32_t a, uint32_t b) {
    (void)a; (void)b;
    vga_puts("[SYSCALL] exit status "); vga_putdec(status); vga_puts("\n");
    serial_puts("[SYSCALL] exit "); serial_puthex(status); serial_puts("\n");
    struct task* cur = task_current();
    if (cur) cur->exit_status = (int)status;
    task_exit(); // Dönmeyecek
    return 0;
}

static uint32_t sys_yield(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    schedule();
    return 0;
}

static uint32_t sys_getticks(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    return timer_get_ticks();
}

/* 20A: Process listesi — "pid name state prio uid gid\n" satırları.
 * input.c'deki is_user_buf_valid ve task_list arayüzünü kullanır. */
static void tasklist_dec(char* buf, uint32_t* pos, uint32_t max,
                         uint32_t v) {
    char rev[11];
    int r = 0;
    if (v == 0) rev[r++] = '0';
    while (v && r < 10) { rev[r++] = '0' + (v % 10); v /= 10; }
    while (r > 0 && *pos < max - 1) buf[(*pos)++] = rev[--r];
}
static void tasklist_str(char* buf, uint32_t* pos, uint32_t max,
                         const char* s) {
    while (*s && *pos < max - 1) buf[(*pos)++] = *s++;
}
static uint32_t sys_tasklist(uint32_t ubuf, uint32_t maxlen, uint32_t c) {
    (void)c;
    extern struct task* task_list;
    if (maxlen == 0 || maxlen > 2048) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, (uint32_t)maxlen)) return (uint32_t)-1;
    static char kbuf[2048];
    uint32_t pos = 0;
    struct task* t = task_list;
    while (t && pos + 32 < maxlen) {
        tasklist_dec(kbuf, &pos, maxlen, (uint32_t)t->pid);
        tasklist_str(kbuf, &pos, maxlen, " ");
        tasklist_str(kbuf, &pos, maxlen, t->name);
        tasklist_str(kbuf, &pos, maxlen, " state=");
        tasklist_dec(kbuf, &pos, maxlen, (uint32_t)t->state);
        tasklist_str(kbuf, &pos, maxlen, " prio=");
        tasklist_dec(kbuf, &pos, maxlen, (uint32_t)t->priority);
        tasklist_str(kbuf, &pos, maxlen, " uid=");
        tasklist_dec(kbuf, &pos, maxlen, (uint32_t)t->uid);
        tasklist_str(kbuf, &pos, maxlen, " gid=");
        tasklist_dec(kbuf, &pos, maxlen, (uint32_t)t->gid);
        tasklist_str(kbuf, &pos, maxlen, "\n");
        t = t->next;
    }
    if (pos > 0) {
        kbuf[pos] = 0;
        if (copy_to_user(ubuf, kbuf, (uint32_t)(pos + 1)) != 0) return (uint32_t)-1;
    }
    return (uint32_t)(pos + 1);
}

static uint32_t sys_open(uint32_t path, uint32_t mode, uint32_t a) {
    (void)a;
    const char* p = (const char*)path;
    // Basit: kullanıcı string adresi doğrula
    if (!path || !paging_get_phys(path)) return (uint32_t)-1;
    return (uint32_t)fs_open(p, (int)mode);
}

static uint32_t sys_close(uint32_t fd, uint32_t a, uint32_t b) {
    (void)a; (void)b;
    return (uint32_t)fs_close((int)fd);
}

static uint32_t sys_fork(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    if (!syscall_cur_regs) return (uint32_t)-1;
    struct task* child = fork_task(syscall_cur_regs);
    return (child) ? (uint32_t)child->pid : (uint32_t)-1;
}

/* 12D: Blocking pipe syscall - pipefd[2] için iki fd oluştur */
static uint32_t sys_pipe(uint32_t pipefd, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    if (!pipefd || !is_user_buf_valid(pipefd, 8)) return (uint32_t)-1;
    struct pipe* p = pipe_create();
    if (!p) return (uint32_t)-1;
    struct task* cur = task_current();
    if (!cur) { pipe_destroy(p); return (uint32_t)-1; }
    int fd0 = -1, fd1 = -1;
    for (int i = 0; i < 16; i++) if (!cur->fd_table[i].valid) { fd0 = i; break; }
    if (fd0 < 0) { pipe_destroy(p); return (uint32_t)-1; }
    cur->fd_table[fd0].valid = 1;
    cur->fd_table[fd0].type = FD_PIPE;
    cur->fd_table[fd0].pipe = p;
    cur->fd_table[fd0].pipe_write = 0; // read end
    cur->fd_table[fd0].offset = 0;
    for (int i = fd0+1; i < 16; i++) if (!cur->fd_table[i].valid) { fd1 = i; break; }
    if (fd1 < 0) {
        for (int i = 0; i < 16; i++) if (i != fd0 && !cur->fd_table[i].valid) { fd1 = i; break; }
    }
    if (fd1 < 0 || fd1 == fd0) {
        cur->fd_table[fd0].valid = 0;
        pipe_destroy(p);
        return (uint32_t)-1;
    }
    cur->fd_table[fd1].valid = 1;
    cur->fd_table[fd1].type = FD_PIPE;
    cur->fd_table[fd1].pipe = p;
    cur->fd_table[fd1].pipe_write = 1; // write end
    cur->fd_table[fd1].offset = 0;
    // pipe zaten 1 reader/writer ile oluşturuldu, ref_count=2
    int fds[2] = {fd0, fd1};
    if (copy_to_user(pipefd, fds, 8) != 0) {
        cur->fd_table[fd0].valid = 0;
        cur->fd_table[fd1].valid = 0;
        pipe_destroy(p);
        return (uint32_t)-1;
    }
    return 0;
}

static uint32_t sys_exec(uint32_t path, uint32_t uargs, uint32_t b) {
    (void)b;
    if (!path) return (uint32_t)-1;
    // Kullanıcı string'ini bounce buffer'a kopyala (güvenli)
    char kpath[256];
    // is_user_buf_valid için path'in ilk sayfasını kontrol et
    if (!paging_get_phys(path)) return (uint32_t)-1;
    // String uzunluğunu bul (max 255)
    int len = 0;
    const char* upath = (const char*)path;
    while (len < 255) {
        if (!paging_get_phys(path + len)) break;
        if (upath[len] == '\0') break;
        len++;
    }
    if (len >= 255) return (uint32_t)-1;
    if (copy_from_user(path, kpath, len+1) != 0) return (uint32_t)-1;
    kpath[len] = '\0';
    /* 14E: çalıştırma biti gerekli (root muaf) */
    extern int fs_can_path(const char* path, int req);
    if (fs_can_path(kpath, PERM_X) != 0) return (uint32_t)-1;
    /* 19A: argüman dizesi (0 = yok, en fazla 511 bayt) */
    char kargs[512];
    const char* pargs = 0;
    if (uargs) {
        if (!paging_get_phys(uargs)) return (uint32_t)-1;
        const char* ua = (const char*)uargs;
        int alen = 0;
        while (alen < 511) {
            if (!paging_get_phys(uargs + alen)) break;
            if (ua[alen] == '\0') break;
            alen++;
        }
        if (alen >= 511) return (uint32_t)-1;
        if (copy_from_user(uargs, kargs, alen + 1) != 0) return (uint32_t)-1;
        kargs[alen] = '\0';
        pargs = kargs;
    }
    extern int exec_flat(const char* p, const char* args);
    int ret = exec_flat(kpath, pargs);
    return (uint32_t)ret;
}

static uint32_t sys_nanosleep(uint32_t ticks, uint32_t a, uint32_t b) {
    (void)a; (void)b;
    if (ticks == 0) {
        schedule();
        return 0;
    }
    if (ticks > 10000) ticks = 10000; // cap 100 sec
    uint32_t wake = timer_get_ticks() + ticks;
    sleep_until(wake);
    return 0;
}

static uint32_t sys_sem_init(uint32_t value, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int sem_create(int v);
    return (uint32_t)sem_create((int)value);
}
static uint32_t sys_sem_wait(uint32_t sem_id, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int sem_wait_id(int id);
    return (uint32_t)sem_wait_id((int)sem_id);
}
static uint32_t sys_sem_post(uint32_t sem_id, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int sem_post_id(int id);
    return (uint32_t)sem_post_id((int)sem_id);
}
static uint32_t sys_sem_destroy(uint32_t sem_id, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int sem_destroy_id(int id);
    return (uint32_t)sem_destroy_id((int)sem_id);
}
static uint32_t sys_nice(uint32_t inc, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* cur = task_current();
    if (!cur) return (uint32_t)-1;
    int new_prio = cur->priority + (int)inc;
    if (new_prio < 1) new_prio = 1;
    if (new_prio > 3) new_prio = 3;
    cur->priority = new_prio;
    cur->ticks_left = (new_prio + 1) * 5;
    return (uint32_t)new_prio;
}
static uint32_t sys_brk_wrap(uint32_t new_brk, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern uint32_t sys_brk(uint32_t nb);
    return sys_brk(new_brk);
}

/* 14E: kimlik + izinler */
static uint32_t sys_getuid_wrap(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return 0;
    return (uint32_t)t->uid;
}

static uint32_t sys_setuid_wrap(uint32_t uid, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    if ((int)uid == t->uid) return (uint32_t)t->uid; /* değişiklik yok */
    /* 22.5: uid değişimi CAP_SETUID gerektirir (uid==0 kontrolü yerine) */
    if (!cap_has(t->caps, CAP_SETUID)) {
        cap_audit(CAP_SETUID, 0);
        return (uint32_t)-1;
    }
    int was_root = (t->uid == 0);
    t->uid = (int)uid;
    if (was_root && t->uid != 0) {
        /* 22.5: root bırakılınca yetkiler düşer (yükselme engeli) */
        cap_drop_all(&t->caps);
    }
    cap_audit(CAP_SETUID, 1);
    return (uint32_t)t->uid;
}

static uint32_t sys_getgid_wrap(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return 0;
    return (uint32_t)t->gid;
}

static uint32_t sys_setgid_wrap(uint32_t gid, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    if ((int)gid == t->gid) return (uint32_t)t->gid; /* değişiklik yok */
    /* 22.5: gid değişimi CAP_SETGID gerektirir */
    if (!cap_has(t->caps, CAP_SETGID)) {
        cap_audit(CAP_SETGID, 0);
        return (uint32_t)-1;
    }
    t->gid = (int)gid;
    cap_audit(CAP_SETGID, 1);
    return (uint32_t)t->gid;
}

/* 20C: ek gruplar (supplementary groups). (buf, size) -> tasarım:
 * size bayt tampon; her gid 4 bayt (int). Dönen: yazılan bayt. */
static uint32_t sys_getgroups_wrap(uint32_t ubuf, uint32_t usize, uint32_t c) {
    (void)c;
    struct task* t = task_current();
    if (!t) return 0;
    if (!ubuf) return (uint32_t)(t->ngroups * 4);
    if (t->ngroups <= 0) return 0;
    uint32_t nbytes = (uint32_t)t->ngroups * 4;
    if (nbytes > usize) nbytes = usize;
    if (nbytes > 128) nbytes = 128;
    if (!is_user_buf_valid(ubuf, nbytes)) return (uint32_t)-1;
    if (copy_to_user(ubuf, (const char*)t->groups, nbytes) != 0) return (uint32_t)-1;
    return nbytes;
}

static uint32_t sys_setgroups_wrap(uint32_t ubuf, uint32_t usize, uint32_t c) {
    (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    if (!cap_has(t->caps, CAP_SETGID)) return (uint32_t)-1; /* 22.5: sadece yetkili */
    if (!ubuf || usize == 0) {
        t->ngroups = 0;
        return 0;
    }
    if (usize > TASK_NGROUPS * 4) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, usize)) return (uint32_t)-1;
    int32_t kg[TASK_NGROUPS];
    if (copy_from_user(ubuf, (char*)kg, usize) != 0) return (uint32_t)-1;
    int cnt = (int)(usize / 4);
    for (int i = 0; i < cnt; i++) t->groups[i] = kg[i];
    t->ngroups = cnt;
    return 0;
}

static uint32_t sys_capget_wrap(uint32_t pid, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return 0;
    if (pid != 0) {
        extern struct task* task_list;
        struct task* k = task_list;
        while (k && k->pid != (int)pid) k = k->next;
        if (!k) return (uint32_t)-1;
        return k->caps;
    }
    return t->caps;
}

static uint32_t sys_capset_wrap(uint32_t caps, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    if (t->uid == 0) {
        /* 22.5: root serbest ama geçersiz bitler reddedilir */
        if ((caps & ~CAP_VALID_MASK) != 0) return (uint32_t)-1;
        t->caps = caps;
        cap_audit(caps, 1);
        return t->caps;
    }
    /* 22.5: yetkisiz yalnızca alt kümeye inebilir (cap.c tek doğruluk kaynağı) */
    uint32_t n = cap_allow_only(t->caps, caps);
    if (n != caps) {
        cap_audit(caps, 0);
        return (uint32_t)-1;
    }
    t->caps = n;
    cap_audit(caps, 1);
    return t->caps;
}

/* 23.5: görev sandbox yönetimi (yalnızca kendi filtresi; geri dönüşsüz sıkılaştırma) */
static uint32_t sys_sb_allow_wrap(uint32_t nr, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    return (uint32_t)sb_allow(t->sb_mask, nr);
}

static uint32_t sys_sb_deny_wrap(uint32_t nr, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    return (uint32_t)sb_deny(t->sb_mask, nr);
}

static uint32_t sys_sb_on_wrap(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    struct task* t = task_current();
    if (!t) return (uint32_t)-1;
    t->sb_on = 1;
    return 0;
}

/* 30.4: kayıtlı ama belgesiz syscall sayacı (0=ABI temiz) */
int syscall_abi_check(void) {
    int missing = 0;
    for (uint32_t i = 0; i < 256; i++) {
        if (syscall_table[i] && !doc_name(i)) missing++;
    }
    return missing;
}

static uint32_t sys_auth_wrap(uint32_t token, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    const char* name = "host"; /* 37.6: basit simülasyon */
    uint32_t exp = auth_token_gen(name, 0xDEADBEEF);
    return auth_check(name, token); /* 0 ok / -1 ret */
}

/* 29.4 + 37.4 + 37.7: makine-okunur belgeler + capability denetim günlüğü + audit */
static uint32_t sys_capaudit_wrap(uint32_t idx, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    uint32_t cap, granted;
    struct task* cur = task_current();
    if (!cur) return (uint32_t)-1;
    if (cap_audit_read(idx, &cap, &granted) != 0) return (uint32_t)-1;
    return (uint32_t)(granted ? 1 : 0);
}
static uint32_t sys_docname_wrap(uint32_t nr, uint32_t ubuf, uint32_t max) {
    const char* s = doc_name(nr);
    char kbuf[32];
    int n;
    if (!s) return (uint32_t)-1;
    if (!ubuf || max == 0 || max > sizeof(kbuf)) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, max)) return (uint32_t)-1;
    n = doc_copy(s, kbuf, max);
    if (n < 0) return (uint32_t)-1;
    if (copy_to_user(ubuf, kbuf, (uint32_t)n + 1) != 0) return (uint32_t)-1;
    return (uint32_t)n;
}

static uint32_t sys_docdesc_wrap(uint32_t nr, uint32_t ubuf, uint32_t max) {
    const char* s = doc_desc(nr);
    char kbuf[64];
    int n;
    if (!s) return (uint32_t)-1;
    if (!ubuf || max == 0 || max > sizeof(kbuf)) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, max)) return (uint32_t)-1;
    n = doc_copy(s, kbuf, max);
    if (n < 0) return (uint32_t)-1;
    if (copy_to_user(ubuf, kbuf, (uint32_t)n + 1) != 0) return (uint32_t)-1;
    return (uint32_t)n;
}

static uint32_t sys_chmod_wrap(uint32_t path, uint32_t mode, uint32_t c) {
    (void)c;
    const char* p = (const char*)path;
    if (!path || !paging_get_phys(path)) return (uint32_t)-1;
    char kpath[256];
    int len = 0;
    while (len < 255) {
        if (!paging_get_phys(path + len)) break;
        if (p[len] == '\0') break;
        len++;
    }
    if (len >= 255) return (uint32_t)-1;
    if (copy_from_user(path, kpath, len+1) != 0) return (uint32_t)-1;
    kpath[len] = '\0';
    extern int fs_chmod(const char* path, int mode);
    return (uint32_t)fs_chmod(kpath, (int)mode);
}

/* 14F: syslog + uptime */
static uint32_t sys_syslog_wrap(uint32_t op, uint32_t buf, uint32_t len) {
    extern int syslog_read(char* b, int m);
    extern int syslog_puts(const char* s);
    if (op == 0) { /* oku + temizle */
        char kbuf[512];
        if (len > sizeof(kbuf) - 1) len = sizeof(kbuf) - 1;
        if (len <= 0) return 0;
        int n = syslog_read(kbuf, (int)len + 1);
        if (n < 0) return (uint32_t)-1;
        if (!is_user_buf_valid(buf, (uint32_t)n)) return (uint32_t)-1;
        if (n > 0 && copy_to_user(buf, kbuf, (uint32_t)n) != 0)
            return (uint32_t)-1;
        return (uint32_t)n;
    } else if (op == 1) { /* yaz */
        char kbuf[512];
        if (len > sizeof(kbuf) - 1) len = sizeof(kbuf) - 1;
        if (len == 0) return 0;
        if (copy_from_user(buf, kbuf, len) != 0) return (uint32_t)-1;
        kbuf[len] = '\0';
        int n = syslog_puts(kbuf);
        return (uint32_t)n;
    }
    return (uint32_t)-1;
}

static uint32_t sys_uptime_wrap(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    return timer_get_ticks() / 100;
}

static uint32_t sys_dup2_wrap(uint32_t oldfd, uint32_t newfd, uint32_t c) {
    (void)c;
    return (uint32_t)fs_dup2((int)oldfd, (int)newfd);
}

static uint32_t sys_futex_wrap(uint32_t uaddr, uint32_t op, uint32_t val) {
    extern int futex_wait(uint32_t u, uint32_t v);
    extern int futex_wake(uint32_t u, int n);
    if (op == 0) { /* FUTEX_WAIT */
        int r = futex_wait(uaddr, val);
        return (uint32_t)r;
    } else if (op == 1) { /* FUTEX_WAKE */
        int r = futex_wake(uaddr, (int)val);
        return (uint32_t)r;
    }
    return (uint32_t)-1;
}

static uint32_t sys_mmap_wrap(uint32_t addr, uint32_t len, uint32_t prot_flags) {
    // prot_flags: low 8 prot, high 8 flags+fd/offset not used for now
    // Simplified: addr=len, prot, flags, fd, offset via extra regs? Use bounce
    // For now, direct call to sys_mmap with addr,len,prot,flags
    extern void* sys_mmap(uint32_t a, uint32_t l, int p, int f, int fd, uint32_t off);
    int prot = prot_flags & 0xFF;
    int flags = (prot_flags >> 8) & 0xFF;
    return (uint32_t)sys_mmap(addr, len, prot, flags, -1, 0);
}
static uint32_t sys_munmap_wrap(uint32_t addr, uint32_t len, uint32_t c) {
    (void)c;
    extern int sys_munmap(uint32_t a, uint32_t l);
    return (uint32_t)sys_munmap(addr, len);
}
/* 13F: thread clone - aynı address space (cr3 paylaşımı), yeni kernel stack */
static uint32_t sys_clone(uint32_t entry, uint32_t stack, uint32_t c) {
    (void)c;
    if (!entry || !stack) return (uint32_t)-1;
    extern struct task* task_clone_thread(uint32_t entry, uint32_t stack);
    struct task* th = task_clone_thread(entry, stack);
    return th ? (uint32_t)th->pid : (uint32_t)-1;
}

static uint32_t sys_signal(uint32_t sig, uint32_t handler, uint32_t c) {
    (void)c;
    extern int sys_signal_handler(int s, uint32_t h);
    return (uint32_t)sys_signal_handler((int)sig, handler);
}
static uint32_t sys_kill(uint32_t pid, uint32_t sig, uint32_t c) {
    (void)c;
    /* 14E: aynı uid ya da root gerekli */
    struct task* cur = task_current();
    if (cur && cur->uid != 0) {
        extern struct task* task_list;
        struct task* t = task_list;
        int found_uid = -1;
        while (t) {
            if (t->pid == (int)pid) { found_uid = t->uid; break; }
            t = t->next;
        }
        if (found_uid < 0 || found_uid != cur->uid) return (uint32_t)-1;
    }
    extern int sys_kill_handler(int p, int s);
    return (uint32_t)sys_kill_handler((int)pid, (int)sig);
}
static uint32_t sys_sigreturn(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    extern int sys_sigreturn_handler(void);
    return (uint32_t)sys_sigreturn_handler();
}
static uint32_t sys_waitpid(uint32_t pid, uint32_t status, uint32_t options) {
    int st = 0;
    extern int wait_task(int p, int* status);
    extern int wait_task_wnohang(int p, int* status);
    int ret;
    if (options & WNOHANG) ret = wait_task_wnohang((int)pid, &st); /* 19G */
    else ret = wait_task((int)pid, &st);
    if (ret > 0 && status) {
        if (!is_user_buf_valid(status, 4)) return (uint32_t)-1;
        copy_to_user(status, &st, 4);
    }
    return (uint32_t)ret;
}

/* 13G: ham sektör R/W (512B bounce buffer ile) */
static uint32_t sys_blkread(uint32_t lba, uint32_t ubuf, uint32_t c) {
    (void)c;
    extern int blk_read(uint32_t lba, void* buf);
    if (!is_user_buf_valid(ubuf, 512)) return (uint32_t)-1;
    if (blk_read(lba, bounce_buf) != 0) return (uint32_t)-1;
    if (copy_to_user(ubuf, bounce_buf, 512) != 0) return (uint32_t)-1;
    return 0;
}
static uint32_t sys_blkwrite(uint32_t lba, uint32_t ubuf, uint32_t c) {
    (void)c;
    /* 22.5: ham disk yazma CAP_SYS_RAWIO gerektirir */
    struct task* wt = task_current();
    if (!wt || !cap_has(wt->caps, CAP_SYS_RAWIO)) {
        cap_audit(CAP_SYS_RAWIO, 0);
        return (uint32_t)-1;
    }
    extern int blk_write(uint32_t lba, const void* buf);
    if (!is_user_buf_valid(ubuf, 512)) return (uint32_t)-1;
    if (copy_from_user(ubuf, bounce_buf, 512) != 0) return (uint32_t)-1;
    if (blk_write(lba, bounce_buf) != 0) return (uint32_t)-1;
    return 0;
}
static uint32_t sys_blkinfo(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    extern uint32_t blk_capacity(void);
    return blk_capacity();
}
/* 18A: input hub'dan bir olay al (6 byte struct). 1 olay / 0 boş. */
static uint32_t sys_input(uint32_t uev, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int input_pop(struct input_event* ev);
    if (!is_user_buf_valid(uev, 6)) return (uint32_t)-1;
    struct input_event kev;
    int r = input_pop(&kev);
    if (r <= 0) return 0;
    if (copy_to_user(uev, &kev, 6) != 0) return (uint32_t)-1;
    return 1;
}
/* 18B: fb modunu al (12 byte {w,h,bpp}). 1 aktif / 0 yok. */
static uint32_t sys_fbinfo(uint32_t umode, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int fb_active(void);
    extern void fb_get_mode(uint32_t* w, uint32_t* h, uint32_t* bpp);
    if (!is_user_buf_valid(umode, 12)) return (uint32_t)-1;
    uint32_t m[3] = {0, 0, 0};
    if (fb_active()) fb_get_mode(&m[0], &m[1], &m[2]);
    else return 0;
    if (copy_to_user(umode, m, 12) != 0) return (uint32_t)-1;
    return 1;
}
/* 13H: diskfs dosya yaz/oku (isim + data bounce buffer ile) */
static uint32_t sys_dfsave(uint32_t uname, uint32_t ubuf, uint32_t len) {
    extern int dfile_write(const char* name, const char* data, int len);
    if (!uname || !paging_get_phys(uname)) return (uint32_t)-1;
    if (len > DISKFS_MAXFILE) return (uint32_t)-1;
    if (len > 0 && !is_user_buf_valid_big(ubuf, len)) return (uint32_t)-1;
    char kname[25];
    int nl = 0;
    const char* up = (const char*)uname;
    while (nl < 24) {
        if (!paging_get_phys(uname + nl)) return (uint32_t)-1;
        if (up[nl] == '\0') break;
        nl++;
    }
    if (nl >= 24) return (uint32_t)-1;
    if (copy_from_user(uname, kname, nl + 1) != 0) return (uint32_t)-1;
    kname[nl] = '\0';
    if (len > 0 && copy_from_user(ubuf, bounce_buf, len) != 0) return (uint32_t)-1;
    return (uint32_t)dfile_write(kname, (const char*)bounce_buf, (int)len);
}
static uint32_t sys_dfload(uint32_t uname, uint32_t ubuf, uint32_t maxlen) {
    extern int dfile_read(const char* name, char* buf, int maxlen);
    if (!uname || !paging_get_phys(uname)) return (uint32_t)-1;
    if (maxlen > DISKFS_MAXFILE) maxlen = DISKFS_MAXFILE; /* 17B: büyük okuma */
    if (maxlen > 0 && !is_user_buf_valid_big(ubuf, maxlen)) return (uint32_t)-1;
    char kname[25];
    int nl = 0;
    const char* up = (const char*)uname;
    while (nl < 24) {
        if (!paging_get_phys(uname + nl)) return (uint32_t)-1;
        if (up[nl] == '\0') break;
        nl++;
    }
    if (nl >= 24) return (uint32_t)-1;
    if (copy_from_user(uname, kname, nl + 1) != 0) return (uint32_t)-1;
    kname[nl] = '\0';
    int n = dfile_read(kname, (char*)bounce_buf, (int)maxlen);
    if (n < 0) return (uint32_t)-1;
    if (n > 0 && copy_to_user(ubuf, bounce_buf, (uint32_t)n) != 0) return (uint32_t)-1;
    return (uint32_t)n;
}

/* 17 serisi: diskfs v2 syscall'ları */
static uint32_t sys_dfmkdir(uint32_t uname, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int df_mkdir(const char* path);
    char kname[25];
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    return (uint32_t)df_mkdir(kname);
}
static uint32_t sys_dfremove(uint32_t uname, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int df_remove(const char* path);
    char kname[25];
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    return (uint32_t)df_remove(kname);
}
static uint32_t sys_dftruncate(uint32_t uname, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int df_truncate(const char* path);
    char kname[25];
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    return (uint32_t)df_truncate(kname);
}
static uint32_t sys_dfstat(uint32_t uname, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    extern int df_stat(const char* path, struct dfs_stat* out);
    char kname[25];
    struct dfs_stat st;
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    if (df_stat(kname, &st) != 0) return (uint32_t)-1;
    if (st.size > 0x7FFFFFFFu) return (uint32_t)-1;
    return (st.is_dir ? 0x80000000u : 0u) | st.size;
}
static uint32_t sys_dflist(uint32_t uname, uint32_t ubuf, uint32_t maxlen) {
    extern int df_readdir(const char* path, char* out, int max);
    char kname[25];
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    if (maxlen == 0 || maxlen > 4096) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, maxlen)) return (uint32_t)-1;
    int n = df_readdir(kname, (char*)bounce_buf, (int)maxlen);
    if (n < 0) return (uint32_t)-1;
    if (n > 0 && copy_to_user(ubuf, bounce_buf, (uint32_t)n) != 0) return (uint32_t)-1;
    return (uint32_t)n;
}
static uint32_t sys_dfcheck(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    extern int df_check(void);
    return (uint32_t)df_check();
}
/* 19J: chfs listeleme — "ad\n" (dizinse "ad/\n") satırları. chfs flat
 * olduğundan path yok sayılır, filtreleme kullanıcıda yapılır. */
static uint32_t sys_lsdir(uint32_t uname, uint32_t ubuf, uint32_t maxlen) {
    extern int chfs_list_dir(const char* path, struct fs_list_entry* out, int max);
    char kname[25];
    static struct fs_list_entry kents[FS_LIST_MAX];
    if (df_copy_path(uname, kname) < 0) return (uint32_t)-1;
    if (maxlen == 0 || maxlen > 4096) return (uint32_t)-1;
    if (!is_user_buf_valid(ubuf, maxlen)) return (uint32_t)-1;
    int n = chfs_list_dir(kname, kents, FS_LIST_MAX);
    if (n < 0) return (uint32_t)-1;
    uint32_t pos = 0;
    for (int i = 0; i < n; i++) {
        const char* nm = kents[i].name;
        int bl = 0;
        while (nm[bl] && bl < FS_LIST_NAME_LEN - 1) bl++;
        if (pos + (uint32_t)bl + 2 > maxlen) break;
        if (pos + (uint32_t)bl + 2 > BOUNCE_SIZE) break;
        for (int k = 0; k < bl; k++) bounce_buf[pos++] = (uint8_t)nm[k];
        if (kents[i].is_dir) bounce_buf[pos++] = '/';
        bounce_buf[pos++] = '\n';
    }
    if (pos > 0 && copy_to_user(ubuf, bounce_buf, pos) != 0) return (uint32_t)-1;
    return pos;
}

/* 19K: Sanal terminal değiştir (1=GUI, 2..4=metin konsolu). */
static uint32_t sys_vtswitch(uint32_t vt, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    if (vt < VT_MIN || vt > VT_MAX) return (uint32_t)-1;
    serial_puts("[VTSWITCH] pid="); serial_puthex(task_current()->pid);
    serial_puts(" -> "); serial_putc((char)('0' + vt)); serial_puts("\n");
    return (uint32_t)vt_switch((int)vt);
}

/* 15C: per-task TLS tabanı — clone/fork taşır (fork.c/clone.c). */
static uint32_t sys_tls_get(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    struct task* cur = task_current();
    return cur ? cur->user_tls : 0;
}

static uint32_t sys_tls_set(uint32_t addr, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    struct task* cur = task_current();
    if (!cur) return (uint32_t)-1;
    uint32_t old = cur->user_tls;
    cur->user_tls = addr;
    return old;
}

/* Syscall dispatcher - C'den çağrılır */
void syscall_handler(struct registers* regs) {
    syscall_cur_regs = regs;
    uint32_t call_no = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    // Debug: sessiz (12A/12B doğrulaması için serial'da sadece [12A]/[12B] logları)

    // Syscall sayısı kontrolü (13F: CLONE=130 için 256)
    if (call_no >= 256 || !syscall_table[call_no]) {
        vga_puts("[SYSCALL] Unknown "); vga_putdec(call_no);
        vga_puts(" table="); vga_puthex((uint32_t)syscall_table[call_no]); vga_puts("\n");
        serial_puts("[SYSCALL] Unknown "); serial_puthex(call_no); serial_puts("\n");
        regs->eax = (uint32_t)-1;
        return;
    }
    /* 23.5: görev sandbox zoru (varsayılan kapalı; açan görev maskeye uyar) */
    {
        struct task* st = task_current();
        if (st && st->sb_on && !sb_check(st->sb_mask, call_no)) {
            regs->eax = (uint32_t)-1;
            return;
        }
    }
    regs->eax = syscall_table[call_no](arg1, arg2, arg3);
}

void syscall_init(void) {
    // Tabloyu kur
    syscall_table[SYS_EXIT]  = sys_exit;
    syscall_table[SYS_WRITE] = sys_write;
    syscall_table[SYS_READ]  = sys_read;
    syscall_table[SYS_GETPID]= sys_getpid;
    syscall_table[SYS_YIELD] = sys_yield;
    syscall_table[SYS_GETTICKS] = sys_getticks;
    syscall_table[SYS_OPEN] = sys_open;
    syscall_table[SYS_CLOSE] = sys_close;
    syscall_table[SYS_FORK] = sys_fork;
    syscall_table[SYS_PIPE] = sys_pipe;
    syscall_table[SYS_SIGNAL] = sys_signal;
    syscall_table[SYS_EXEC] = sys_exec;
    syscall_table[SYS_NANOSLEEP] = sys_nanosleep;
    syscall_table[SYS_SEM_INIT] = sys_sem_init;
    syscall_table[SYS_SEM_WAIT] = sys_sem_wait;
    syscall_table[SYS_SEM_POST] = sys_sem_post;
    syscall_table[SYS_SEM_DESTROY] = sys_sem_destroy;
    syscall_table[SYS_KILL] = sys_kill;
    syscall_table[SYS_SIGRETURN] = sys_sigreturn;
    syscall_table[SYS_WAITPID] = sys_waitpid;
    syscall_table[SYS_MMAP] = (syscall_fn_t)sys_mmap_wrap;
    syscall_table[SYS_MUNMAP] = (syscall_fn_t)sys_munmap_wrap;
    syscall_table[SYS_NICE] = sys_nice;
    syscall_table[SYS_CLONE] = sys_clone;
    syscall_table[SYS_BLKREAD] = sys_blkread;
    syscall_table[SYS_BLKWRITE] = sys_blkwrite;
    syscall_table[SYS_BLKINFO] = sys_blkinfo;
    syscall_table[SYS_DFSAVE] = sys_dfsave;
    syscall_table[SYS_DFLOAD] = sys_dfload;
    syscall_table[SYS_INPUT] = sys_input;
    syscall_table[SYS_FBINFO] = sys_fbinfo;
    syscall_table[SYS_BRK] = sys_brk_wrap;
    syscall_table[SYS_FUTEX] = sys_futex_wrap;
    syscall_table[SYS_DUP2] = sys_dup2_wrap;
    syscall_table[SYS_GETUID] = sys_getuid_wrap;
    syscall_table[SYS_SETUID] = sys_setuid_wrap;
    syscall_table[SYS_GETGID] = sys_getgid_wrap;
    syscall_table[SYS_SETGID] = sys_setgid_wrap;
    syscall_table[SYS_CHMOD] = sys_chmod_wrap;
    syscall_table[SYS_GETGROUPS] = sys_getgroups_wrap;
    syscall_table[SYS_SETGROUPS] = sys_setgroups_wrap;
    syscall_table[SYS_CAPGET] = sys_capget_wrap;
    syscall_table[SYS_CAPSET] = sys_capset_wrap;
    syscall_table[SYS_SB_ALLOW] = sys_sb_allow_wrap; /* 23.5 */
    syscall_table[SYS_SB_DENY] = sys_sb_deny_wrap;
    syscall_table[SYS_SB_ON] = sys_sb_on_wrap;
    syscall_table[SYS_DOCNAME] = sys_docname_wrap; /* 29.4 */
    syscall_table[SYS_DOCDESC] = sys_docdesc_wrap;
    syscall_table[SYS_CAPAUDIT] = sys_capaudit_wrap; /* 37.7: audit günlüğü okuma */
    syscall_table[SYS_AUTH] = sys_auth_wrap; /* 37.3 */
    syscall_table[SYS_SYSLOG] = sys_syslog_wrap;
    syscall_table[SYS_UPTIME] = sys_uptime_wrap;
    syscall_table[SYS_TLS_SET] = sys_tls_set;
    syscall_table[SYS_TLS_GET] = sys_tls_get;
    syscall_table[SYS_DFMKDIR] = sys_dfmkdir;
    syscall_table[SYS_DFREMOVE] = sys_dfremove;
    syscall_table[SYS_DFTRUNCATE] = sys_dftruncate;
    syscall_table[SYS_DFSTAT] = sys_dfstat;
    syscall_table[SYS_DFLIST] = sys_dflist;
    syscall_table[SYS_DFCHECK] = sys_dfcheck;
    syscall_table[SYS_LSDIR] = sys_lsdir;
    syscall_table[SYS_VTSWITCH] = sys_vtswitch;
    syscall_table[SYS_TASKLIST] = sys_tasklist;

    // int 0x80 handler'ı IDT'ye ekle - TRAP GATE, DPL=3 (user çağırabilir)
    extern void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEE); // 0xEE = 11101110b: P=1, DPL=3, Type=0xE (32-bit interrupt/trap)

    // IDT'yi tekrar yükle? Gerek yok, idt_entries global ve idt_init sonrası yüklü
    // Ama idt_init'den sonra ekleme yaptıysak lidt tekrar çalıştırmalıyız
    // En güvenli: idt yüklendikten sonra syscall_init çağrılacak
    // Burada tekrar yükle
    extern void idt_flush(uint32_t ptr);
    extern struct idt_ptr idt_ptr_reg;
    idt_flush((uint32_t)&idt_ptr_reg);

    vga_puts("[6A] Syscall init (int 0x80, trap gate DPL=3)\n");
    serial_puts("[6A] Syscall init\n");
}