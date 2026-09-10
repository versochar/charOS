#ifndef CHAROS_CORE_SYSCALL_H
#define CHAROS_CORE_SYSCALL_H

#include <stdint.h>
#include <core/isr.h>

/* Syscall numaraları */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_GETPID  3
#define SYS_YIELD   4
#define SYS_GETTICKS 5
#define SYS_OPEN   10
#define SYS_CLOSE  11
#define SYS_FORK   12
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
#define SYS_BLKREAD 131  /* 13G: sektör oku (lba, userbuf512) */
#define SYS_BLKWRITE 132 /* 13G: sektör yaz (lba, userbuf512) */
#define SYS_DFSAVE 133   /* 13H: diskfs dosyaya yaz (name, buf, len) */
#define SYS_DFLOAD 134   /* 13H: diskfs dosyadan oku (name, buf, len) */
#define SYS_BLKINFO 135  /* 13G: toplam sektör sayısı */
#define SYS_INPUT 136    /* 18A: hub'dan olay al (userbuf6, _) */
#define SYS_FBINFO 137   /* 18B: fb modu al (userbuf12 {w,h,bpp}) */
#define SYS_BRK 138      /* 14A: program break (yeni adres / 0=sorgu) */
#define SYS_FUTEX 139    /* 14D: futex bekle/uyandır (uaddr, op, val) */
#define SYS_GETUID 140   /* 14E: uid al */
#define SYS_SETUID 141   /* 14E: uid ayarla (root her şeye, diğerleri kendine) */
#define SYS_GETGID 142   /* 14E: gid al */
#define SYS_SETGID 143   /* 14E: gid ayarla */
#define SYS_SYSLOG 144   /* 14F: halka tampona yaz/oku (op, buf, len) */
#define SYS_UPTIME 145   /* 14F: saniye (ticks/100) */
#define SYS_CHMOD 146    /* 14E: dosya modu (path, mode) */
#define SYS_DUP2 147    /* 14C: fd yönlendirme (oldfd, newfd) */
#define SYS_TLS_SET 148 /* 15C: TLS tabanı ata (addr) -> önceki değer */
#define SYS_TLS_GET 149 /* 15C: TLS tabanını al */
/* 17 serisi: kalıcı disk FS API'leri (diskfs v2) */
#define SYS_DFMKDIR 150   /* 17A: dizin oluştur (path) */
#define SYS_DFREMOVE 151  /* 17C: dosya/dizin sil (path) */
#define SYS_DFTRUNCATE 152 /* 17D: dosyayı 0'la (path) */
#define SYS_DFSTAT 153    /* 17F: boyut/is_dir (path) -> (is_dir<<31)|size / -1 */
#define SYS_DFLIST 154    /* 17F: alt öğeleri listele (dirpath, buf, max) */
#define SYS_DFCHECK 155   /* 17H: bütünlük denetimi -> 0 ok / -1 bozuk */
#define SYS_LSDIR 156     /* 19J: chfs listeleme (dirpath, buf, max) -> bayt / -1 */
#define SYS_VTSWITCH 157  /* 19K: sanal terminali değştir (1=GUI, 2..4=metin) */
#define SYS_TASKLIST 158  /* 20A: process listesi -> satırlar "pid name state prio uid gid" */
/* 20C: ek gruplar + Linux tarzı capability'ler */
#define SYS_GETGROUPS 159 /* (buf, size) -> yazılan gid bayt/bid / -1 */
#define SYS_SETGROUPS 160 /* (buf, size) -> tamamlandı; root gerekir */
#define SYS_CAPGET 161    /* (pid) -> o anki task capability bitmask */
#define SYS_CAPSET 162    /* (caps) -> ayarla; root serbest, diğerleri kendininkinin alt kümesi */
#define SYS_SB_ALLOW 163  /* 23.5: sandbox'ta nr serbest bırak (nr) -> 0 / -1 */
#define SYS_SB_DENY 164   /* 23.5: sandbox'ta nr engelle (nr) -> 0 / -1 */
#define SYS_SB_ON 165     /* 23.5: sandbox zorlamayı aç (geri dönüşsüz) -> 0 */
#define SYS_DOCNAME 166   /* 29.4: syscall adı (nr, buf, max) -> len / -1 */
#define SYS_DOCDESC 167   /* 29.4: syscall açıklaması (nr, buf, max) -> len / -1 */
/* 22.2: capability bitleri process/cap.h'e taşındı (tek doğruluk kaynağı) */
#include <process/cap.h>
#define WNOHANG 1         /* 19G: waitpid bloklanmadan yokla */

void syscall_init(void);
void syscall_handler(struct registers* regs);
struct registers* syscall_get_regs(void);

#endif