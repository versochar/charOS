#ifndef CHAROS_PROCESS_PIPE_H
#define CHAROS_PROCESS_PIPE_H

#include <stdint.h>
#include <core/spinlock.h>

struct task;

/* 9A: Basit pipe (inter-process iletişim) - 12D'de blocking */
#define PIPE_SIZE 512

struct pipe_buf {
    char data[PIPE_SIZE];
    int read_idx;
    int write_idx;
    int full;
};

// 12D: Blocking pipe (per-process fd, TASK_BLOCKED)
struct pipe {
    char data[PIPE_SIZE];
    int read_pos;
    int write_pos;
    int count;
    int readers; // number of read ends open
    int writers; // number of write ends open
    struct task* reader_wait; // single waiter (basitleştirilmiş)
    struct task* writer_wait;
    int ref_count;
    spinlock_t lock;
};

void pipe_init(struct pipe_buf* p);
int pipe_write(struct pipe_buf* p, const char* data, int len);
int pipe_read(struct pipe_buf* p, char* buf, int len);

// 12D API
struct pipe* pipe_create(void);
void pipe_destroy(struct pipe* p);
int pipe_write_block(struct pipe* p, const char* data, int len);
int pipe_read_block(struct pipe* p, char* buf, int len);
void pipe_close_end(struct pipe* p, int is_write);
/* Kısıt-1 düzeltmesi: bloklamayan okuma (boşsa 0) + doluluk sayacı.
 * Okuma, bekleyen yazıcıyı uyandırır (tıkanma çözülür). */
int pipe_read_nonblock(struct pipe* p, char* buf, int len);
int pipe_count(struct pipe* p);
int pipe_api_selftest(void); /* nonblock/count doğrula. 0 PASS. */

#endif
