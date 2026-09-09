/* 14A: libcharos — ortak user kitaplığı (freestanding, Ring3). */
#include "libcharos.h"

lib_u32 lib_syscall(lib_u32 n, lib_u32 a, lib_u32 b, lib_u32 c) {
    lib_u32 ret;
    asm volatile(
        "pushl %%ebx\n\t"
        "pushl %%ecx\n\t"
        "pushl %%edx\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "movl %3, %%edx\n\t"
        "movl %4, %%eax\n\t"
        "int $0x80\n\t"
        "popl %%edx\n\t"
        "popl %%ecx\n\t"
        "popl %%ebx\n\t"
        : "=a"(ret)
        : "r"(a), "r"(b), "r"(c), "r"(n)
        : "memory"
    );
    return ret;
}

void lib_exit(int status) {
    lib_syscall(SYS_EXIT, (lib_u32)status, 0, 0);
    for (;;) asm volatile("hlt");
}

lib_u32 lib_write(int fd, const char* buf, lib_u32 len) {
    return lib_syscall(SYS_WRITE, (lib_u32)fd, (lib_u32)buf, len);
}

lib_u32 lib_read(int fd, char* buf, lib_u32 len) {
    return lib_syscall(SYS_READ, (lib_u32)fd, (lib_u32)buf, len);
}

int lib_getpid(void) {
    return (int)lib_syscall(SYS_GETPID, 0, 0, 0);
}

/* ---- mini string ---- */
lib_u32 lib_strlen(const char* s) {
    lib_u32 n = 0;
    while (s[n]) n++;
    return n;
}

int lib_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char* lib_strcpy(char* dst, const char* src) {
    char* d = dst;
    while ((*d++ = *src++)) { }
    return dst;
}

void* lib_memcpy(void* dst, const void* src, lib_u32 n) {
    char* d = (char*)dst;
    const char* s = (const char*)src;
    for (lib_u32 i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void* lib_memset(void* s, int c, lib_u32 n) {
    char* p = (char*)s;
    for (lib_u32 i = 0; i < n; i++) p[i] = (char)c;
    return s;
}

/* ---- brk heap: bump + free-list (first-fit, birleştiren) ---- */
struct lib_block {
    lib_u32 size;              /* kullanıcı baytı */
    int free;
    struct lib_block* next;
};

static struct lib_block* lib_head = 0;
static lib_u32 lib_brk = 0;    /* bilinen kırılım */

static lib_u32 lib_sbrk(lib_u32 inc) {
    if (lib_brk == 0) {
        lib_brk = lib_syscall(SYS_BRK, 0, 0, 0);
        if (lib_brk == 0) return 0;
    }
    lib_u32 old = lib_brk;
    lib_u32 want = old + inc;
    lib_u32 got = lib_syscall(SYS_BRK, want, 0, 0);
    if (got < want) return 0;
    lib_brk = got;
    return old;
}

void* lib_malloc(lib_u32 size) {
    if (size == 0) size = 1;
    size = (size + 7) & ~7u; /* 8 hizala */
    struct lib_block* b = lib_head;
    struct lib_block* prev = 0;
    while (b) {
        if (b->free && b->size >= size) {
            b->free = 0;
            return (void*)(b + 1);
        }
        prev = b;
        b = b->next;
    }
    lib_u32 total = size + sizeof(struct lib_block);
    lib_u32 addr = lib_sbrk(total);
    if (!addr) return 0;
    b = (struct lib_block*)addr;
    b->size = size;
    b->free = 0;
    b->next = 0;
    if (prev) prev->next = b;
    else lib_head = b;
    return (void*)(b + 1);
}

void lib_free(void* p) {
    if (!p) return;
    struct lib_block* b = ((struct lib_block*)p) - 1;
    b->free = 1;
    /* İleri birleştirme */
    while (b->next && b->next->free &&
           (char*)b + sizeof(struct lib_block) + b->size == (char*)b->next) {
        b->size += sizeof(struct lib_block) + b->next->size;
        b->next = b->next->next;
    }
}

void* lib_calloc(lib_u32 n, lib_u32 size) {
    lib_u32 total = n * size;
    void* p = lib_malloc(total);
    if (p) lib_memset(p, 0, total);
    return p;
}

/* ---- stdio-lite ---- */
int lib_putchar(char c) {
    return (int)lib_write(1, &c, 1);
}

int lib_puts(const char* s) {
    int n = (int)lib_write(1, s, lib_strlen(s));
    lib_putchar('\n');
    return n + 1;
}

static void lib_putdec(lib_u32 v) {
    char buf[11];
    int i = 0;
    if (v == 0) buf[i++] = '0';
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) lib_putchar(buf[--i]);
}

static void lib_puthex(lib_u32 v) {
    const char* h = "0123456789ABCDEF";
    int started = 0;
    for (int sh = 28; sh >= 0; sh -= 4) {
        int d = (v >> sh) & 0xF;
        if (d || started || sh == 0) { lib_putchar(h[d]); started = 1; }
    }
}

int lib_printf(const char* fmt, ...) {
    /* Basit varargs: ilk arg fmt sonrası yığında (cdecl, 32-bit) */
    const lib_u32* ap = (const lib_u32*)(&fmt + 1);
    int count = 0;
    for (const char* p = fmt; *p; p++) {
        if (*p != '%') { lib_putchar(*p); count++; continue; }
        p++;
        if (*p == 'd' || *p == 'u') {
            lib_u32 v = *ap++;
            /* ondalık basamak say */
            lib_u32 t = v, digits = 1;
            while (t >= 10) { t /= 10; digits++; }
            lib_putdec(v);
            count += digits;
        } else if (*p == 'x') {
            lib_u32 v = *ap++;
            lib_u32 t = v, digits = 1;
            while (t >= 16) { t /= 16; digits++; }
            lib_puthex(v);
            count += digits;
        } else if (*p == 's') {
            const char* s = (const char*)*ap++;
            int n = (int)lib_strlen(s);
            lib_write(1, s, n);
            count += n;
        } else if (*p == 'c') {
            lib_putchar((char)*ap++);
            count++;
        } else if (*p == '%') {
            lib_putchar('%');
            count++;
        } else {
            lib_putchar('%');
            lib_putchar(*p);
            count += 2;
        }
    }
    return count;
}
