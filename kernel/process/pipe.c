#include <process/pipe.h>
#include <process/task.h>
#include <memory/kheap.h>
#include <string.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

void pipe_init(struct pipe_buf* p) {
    memset(p, 0, sizeof(struct pipe_buf));
}

int pipe_write(struct pipe_buf* p, const char* data, int len) {
    if (!p || len <= 0 || len > PIPE_SIZE) return -1;
    int written = 0;
    for (int i = 0; i < len && p->write_idx < PIPE_SIZE; i++) {
        p->data[p->write_idx++] = data[i];
        written++;
    }
    if (p->write_idx >= PIPE_SIZE) p->full = 1;
    return written;
}

int pipe_read(struct pipe_buf* p, char* buf, int len) {
    if (!p || len <= 0) return -1;
    int read = 0;
    while (read < len && p->read_idx < p->write_idx) {
        buf[read++] = p->data[p->read_idx++];
    }
    if (p->read_idx >= p->write_idx) p->full = 0;
    return read;
}

// 12D: Blocking pipe implementation
struct pipe* pipe_create(void) {
    struct pipe* p = (struct pipe*)kmalloc(sizeof(struct pipe));
    if (!p) return 0;
    memset(p, 0, sizeof(struct pipe));
    p->read_pos = 0;
    p->write_pos = 0;
    p->count = 0;
    p->readers = 1;
    p->writers = 1;
    p->reader_wait = 0;
    p->writer_wait = 0;
    p->lock.locked = 0;
    return p;
}

void pipe_destroy(struct pipe* p) {
    if (!p) return;
    kfree(p);
}

int pipe_write_block(struct pipe* p, const char* data, int len) {
    if (!p || !data || len <= 0) return -1;
    if (len > PIPE_SIZE) len = PIPE_SIZE;
    int written = 0;
    while (written < len) {
        uint32_t flags;
        spin_lock_irqsave(&p->lock, &flags);
        while (p->count == PIPE_SIZE) {
            if (p->readers == 0) {
                spin_unlock_irqrestore(&p->lock, flags);
                return -1; // broken pipe
            }
            // Block writer
            p->writer_wait = task_current();
            task_current()->state = TASK_BLOCKED;
            spin_unlock_irqrestore(&p->lock, flags);
            schedule();
            spin_lock_irqsave(&p->lock, &flags);
        }
        // Write one byte (or more if space)
        while (written < len && p->count < PIPE_SIZE) {
            p->data[p->write_pos] = data[written];
            p->write_pos = (p->write_pos + 1) % PIPE_SIZE;
            p->count++;
            written++;
        }
        // Wake reader if waiting
        if (p->reader_wait) {
            p->reader_wait->state = TASK_READY;
            p->reader_wait = 0;
        }
        spin_unlock_irqrestore(&p->lock, flags);
        if (written < len) {
            // Need to block again for remaining data, but for simplicity return partial
            // In real pipe, we would loop, but for test we return what we wrote
            // To avoid busy loop, we already handled blocking, so we can continue
        }
    }
    return written;
}

int pipe_read_block(struct pipe* p, char* buf, int len) {
    if (!p || !buf || len <= 0) return -1;
    int read = 0;
    while (read == 0) {
        uint32_t flags;
        spin_lock_irqsave(&p->lock, &flags);
        while (p->count == 0) {
            if (p->writers == 0) {
                spin_unlock_irqrestore(&p->lock, flags);
                return 0; // EOF
            }
            p->reader_wait = task_current();
            task_current()->state = TASK_BLOCKED;
            spin_unlock_irqrestore(&p->lock, flags);
            schedule();
            spin_lock_irqsave(&p->lock, &flags);
        }
        while (read < len && p->count > 0) {
            buf[read] = p->data[p->read_pos];
            p->read_pos = (p->read_pos + 1) % PIPE_SIZE;
            p->count--;
            read++;
        }
        if (p->writer_wait && p->count < PIPE_SIZE) {
            p->writer_wait->state = TASK_READY;
            p->writer_wait = 0;
        }
        spin_unlock_irqrestore(&p->lock, flags);
        if (read > 0) break;
    }
    return read;
}

/* Kısıt-1: bloklamayan okuma. Boşsa 0 döner, asla uyutmaz.
 * Bekleyen yazıcı varsa uyandırır (drain -> tıkanma çözülür). */
int pipe_read_nonblock(struct pipe* p, char* buf, int len) {
    if (!p || !buf || len <= 0) return -1;
    uint32_t flags;
    spin_lock_irqsave(&p->lock, &flags);
    int read = 0;
    while (read < len && p->count > 0) {
        buf[read] = p->data[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_SIZE;
        p->count--;
        read++;
    }
    if (read > 0 && p->writer_wait && p->count < PIPE_SIZE) {
        p->writer_wait->state = TASK_READY;
        p->writer_wait = 0;
    }
    spin_unlock_irqrestore(&p->lock, flags);
    return read;
}

int pipe_count(struct pipe* p) {
    if (!p) return -1;
    uint32_t flags;
    spin_lock_irqsave(&p->lock, &flags);
    int c = p->count;
    spin_unlock_irqrestore(&p->lock, flags);
    return c;
}

/* nonblock/count API doğrulaması. Scheduler öncesi güvenli (bloklama yok). */
int pipe_api_selftest(void) {
    int ok = 1;
    struct pipe* p = pipe_create();
    if (!p) return -1;
    char tmp[16];
    if (pipe_count(p) != 0) ok = 0;
    if (pipe_read_nonblock(p, tmp, sizeof(tmp)) != 0) ok = 0; /* boş -> 0 */
    if (pipe_read_nonblock(0, tmp, 4) != -1) ok = 0;
    if (pipe_write_block(p, "ABCD", 4) != 4) ok = 0;
    if (pipe_count(p) != 4) ok = 0;
    int n = pipe_read_nonblock(p, tmp, 2);
    if (n != 2 || tmp[0] != 'A' || tmp[1] != 'B') ok = 0;
    if (pipe_count(p) != 2) ok = 0;
    n = pipe_read_nonblock(p, tmp, sizeof(tmp));
    if (n != 2 || tmp[0] != 'C' || tmp[1] != 'D') ok = 0;
    if (pipe_count(p) != 0) ok = 0;
    pipe_destroy(p);
    if (ok) {
        serial_puts("[PIPE] nonblock api [PASS]\n");
        vga_puts("[PIPE] nonblock api [PASS]\n");
        return 0;
    }
    serial_puts("[PIPE] nonblock api [FAIL]\n");
    vga_puts("[PIPE] nonblock api [FAIL]\n");
    return -1;
}

void pipe_close_end(struct pipe* p, int is_write) {
    if (!p) return;
    uint32_t flags;
    spin_lock_irqsave(&p->lock, &flags);
    if (is_write) {
        if (p->writers > 0) p->writers--;
        if (p->reader_wait) {
            p->reader_wait->state = TASK_READY;
            p->reader_wait = 0;
        }
    } else {
        if (p->readers > 0) p->readers--;
        if (p->writer_wait) {
            p->writer_wait->state = TASK_READY;
            p->writer_wait = 0;
        }
    }
    int should_free = (p->readers == 0 && p->writers == 0);
    spin_unlock_irqrestore(&p->lock, flags);
    if (should_free) {
        pipe_destroy(p);
    }
}
