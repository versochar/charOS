#ifndef CHAROS_PROCESS_PTY_H
#define CHAROS_PROCESS_PTY_H

/* 14B: PTY + satır disiplini (line discipline).
 * Her PTY: ham giriş halkası + kanonik satır tamponu + yankı/çıktı halkası.
 * Bayraklar: ICANON (satır modu), ECHO (yankı), ONLCR (\n -> \r\n).
 * Engelleyici çağrı yok (selftest/boot ve task bağlamında güvenli). */

#define PTY_ICANON 1
#define PTY_ECHO   2
#define PTY_ONLCR  4

int pty_create(int flags);                    /* id / -1 dolu */
int pty_destroy(int id);                      /* 0 ok */
int pty_set_flags(int id, int flags);         /* 0 ok */
int pty_slave_input(int id, char c);          /* üretici -> ldisc, 0 ok */
int pty_slave_read(int id, char* buf, int max);  /* kanonik satır/ham bayt, 0=yok */
int pty_master_read(int id, char* buf, int max); /* yankı+çıktı, 0=boş */
int pty_master_write(int id, const char* s, int len); /* program çıktısı */
int pty_selftest(void);                       /* ldisc + yankı + ONLCR. 0 PASS. */

#endif
