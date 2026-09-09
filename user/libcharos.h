#ifndef LIBCHAROS_H
#define LIBCHAROS_H

/* 14A: ortak user kitaplığı — syscall sarmalayıcılar + brk heap + printf.
 * Tüm simgeler lib_ önekli (mevcut programlarla çakışmaz). Sayılar
 * kernel include/core/syscall.h ile aynıdır; #ifndef korumalı. */

typedef unsigned int lib_u32;
typedef int lib_i32;

#ifndef SYS_EXIT
#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_GETPID 3
#define SYS_YIELD 4
#define SYS_GETTICKS 5
#define SYS_OPEN 10
#define SYS_CLOSE 11
#define SYS_FORK 12
#define SYS_PIPE 13
#define SYS_SIGNAL 14
#define SYS_EXEC 15
#define SYS_NANOSLEEP 16
#define SYS_KILL 17
#define SYS_SIGRETURN 18
#define SYS_WAITPID 19
#define SYS_SEM_INIT 20
#define SYS_SEM_WAIT 21
#define SYS_SEM_POST 22
#define SYS_SEM_DESTROY 23
#define SYS_MMAP 90
#define SYS_MUNMAP 91
#define SYS_NICE 24
#define SYS_CLONE 130
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
#endif

lib_u32 lib_syscall(lib_u32 n, lib_u32 a, lib_u32 b, lib_u32 c);
void lib_exit(int status);
lib_u32 lib_write(int fd, const char* buf, lib_u32 len);
lib_u32 lib_read(int fd, char* buf, lib_u32 len);
int lib_getpid(void);

/* brk heap (tek iş parçacıklı v1; thread'ler haricen serileştirir) */
void* lib_malloc(lib_u32 size);
void lib_free(void* p);
void* lib_calloc(lib_u32 n, lib_u32 size);

/* stdio-lite */
int lib_putchar(char c);
int lib_puts(const char* s);
int lib_printf(const char* fmt, ...);

/* mini string */
lib_u32 lib_strlen(const char* s);
int lib_strcmp(const char* a, const char* b);
char* lib_strcpy(char* dst, const char* src);
void* lib_memcpy(void* dst, const void* src, lib_u32 n);
void* lib_memset(void* s, int c, lib_u32 n);

#endif
