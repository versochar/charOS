#ifndef CHAROS_DRIVERS_WSERVER_H
#define CHAROS_DRIVERS_WSERVER_H

#include <stdint.h>

/* 18F: Window server - server/client ayrığı.
 * Client: ring-3 program, pipe üstünden WM komut baytları yazar.
 * Server: kernel thread (`wserver_task`), komutları parse edip WM'ye uygular
 * ve tek-byte cevap (0=OK, 0xFF=ERR) döner. */

int wserver_init(void);
void wserver_start_task(void);      /* task sistemi kurulduktan sonra */
int wserver_selftest(void); /* PIPE üstünden client request/response. 0 PASS. */

/* Komut byte'ları (client -> server) */
#define WCMD_CREATE   1   /* +w lo/hi +h lo/hi +bg lo/hi  (6 arg byte) */
#define WCMD_MOVE     2   /* +id +x lo/hi +y lo/hi */
#define WCMD_FOCUS    3   /* +id */
#define WCMD_FILL     4   /* +id +color lo/hi (SADECE client bg) */
#define WCMD_RESIZE   5   /* +id +w lo/hi +h lo/hi */
#define WCMD_MINIMIZE 6   /* +id */
#define WCMD_RESTORE  7   /* +id */
#define WCMD_DESTROY  8   /* +id */
#define WCMD_CLOSE    9   /* +id (destroy + kapat) */
#define WCMD_QUERY    10  /* +id -> X/Y/W/H/flags 5 byte */

#define W_RESP_OK  0x00
#define W_RESP_ERR 0xFF

#endif
