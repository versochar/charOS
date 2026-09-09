#include <process/pty.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14B: PTY + satır disiplini. Kilit başına-PT Y (IRQ/task güvenli). */

#define PTY_MAX   8
#define PTY_RING  512
#define PTY_LINE  256

struct pty {
    int used;
    int flags;
    char in_raw[PTY_RING];   /* kanonik-dışı kuyruk */
    int in_head, in_tail, in_count;
    char canon[PTY_LINE];    /* kanonik satır */
    int canon_len;
    int line_ready;
    char out[PTY_RING];      /* yankı + program çıktısı */
    int out_head, out_tail, out_count;
    spinlock_t lock;
};

static struct pty ptys[PTY_MAX];
static int pty_inited = 0;

static void pty_init_all(void) {
    if (pty_inited) return;
    memset(ptys, 0, sizeof(ptys));
    pty_inited = 1;
}

static struct pty* pty_get(int id) {
    if (id < 0 || id >= PTY_MAX) return 0;
    if (!ptys[id].used) return 0;
    return &ptys[id];
}

static void ring_push(char* b, int* head, int* tail, int* count, int cap, char c) {
    (void)tail;
    if (*count >= cap) return; /* dolu: sessizce at */
    b[*head] = c;
    *head = (*head + 1) % cap;
    (*count)++;
}

static int ring_pop(char* b, int* head, int* tail, int* count, int cap) {
    (void)head;
    if (*count <= 0) return -1;
    char c = b[*tail];
    *tail = (*tail + 1) % cap;
    (*count)--;
    return (unsigned char)c;
}

int pty_create(int flags) {
    pty_init_all();
    for (int i = 0; i < PTY_MAX; i++) {
        uint32_t fl;
        spin_lock_irqsave(&ptys[i].lock, &fl);
        if (!ptys[i].used) {
            ptys[i].used = 1;
            ptys[i].flags = flags;
            ptys[i].in_head = ptys[i].in_tail = ptys[i].in_count = 0;
            ptys[i].canon_len = 0;
            ptys[i].line_ready = 0;
            ptys[i].out_head = ptys[i].out_tail = ptys[i].out_count = 0;
            spin_unlock_irqrestore(&ptys[i].lock, fl);
            return i;
        }
        spin_unlock_irqrestore(&ptys[i].lock, fl);
    }
    return -1;
}

int pty_destroy(int id) {
    struct pty* p = pty_get(id);
    if (!p) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    memset(p, 0, sizeof(*p));
    spin_unlock_irqrestore(&p->lock, fl);
    return 0;
}

int pty_set_flags(int id, int flags) {
    struct pty* p = pty_get(id);
    if (!p) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    p->flags = flags;
    spin_unlock_irqrestore(&p->lock, fl);
    return 0;
}

int pty_slave_input(int id, char c) {
    struct pty* p = pty_get(id);
    if (!p) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    if (p->flags & PTY_ICANON) {
        if (c == '\r') c = '\n';
        if (c == '\n') {
            if (p->canon_len < PTY_LINE - 1) p->canon[p->canon_len++] = '\n';
            p->line_ready = 1;
        } else if (c == 0x08 || c == 0x7F) {
            if (p->canon_len > 0) p->canon_len--;
        } else if (c == 0x15) { /* Ctrl-U: satırı sil */
            p->canon_len = 0;
        } else if ((unsigned char)c >= 32 || c == '\t') {
            if (p->canon_len < PTY_LINE - 1) p->canon[p->canon_len++] = c;
        }
        if (p->flags & PTY_ECHO)
            ring_push(p->out, &p->out_head, &p->out_tail, &p->out_count, PTY_RING, c);
    } else {
        ring_push(p->in_raw, &p->in_head, &p->in_tail, &p->in_count, PTY_RING, c);
        if (p->flags & PTY_ECHO)
            ring_push(p->out, &p->out_head, &p->out_tail, &p->out_count, PTY_RING, c);
    }
    spin_unlock_irqrestore(&p->lock, fl);
    return 0;
}

int pty_slave_read(int id, char* buf, int max) {
    struct pty* p = pty_get(id);
    if (!p || !buf || max <= 0) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    int n = 0;
    if (p->flags & PTY_ICANON) {
        if (!p->line_ready) { spin_unlock_irqrestore(&p->lock, fl); return 0; }
        if (p->canon_len + 1 > max) { spin_unlock_irqrestore(&p->lock, fl); return 0; }
        for (int i = 0; i < p->canon_len; i++) buf[i] = p->canon[i];
        n = p->canon_len;
        p->canon_len = 0;
        p->line_ready = 0;
    } else {
        while (n < max) {
            int c = ring_pop(p->in_raw, &p->in_head, &p->in_tail, &p->in_count, PTY_RING);
            if (c < 0) break;
            buf[n++] = (char)c;
        }
    }
    spin_unlock_irqrestore(&p->lock, fl);
    return n;
}

int pty_master_read(int id, char* buf, int max) {
    struct pty* p = pty_get(id);
    if (!p || !buf || max <= 0) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    int n = 0;
    while (n < max) {
        int c = ring_pop(p->out, &p->out_head, &p->out_tail, &p->out_count, PTY_RING);
        if (c < 0) break;
        buf[n++] = (char)c;
    }
    spin_unlock_irqrestore(&p->lock, fl);
    return n;
}

int pty_master_write(int id, const char* s, int len) {
    struct pty* p = pty_get(id);
    if (!p || !s || len < 0) return -1;
    uint32_t fl;
    spin_lock_irqsave(&p->lock, &fl);
    int n = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] == '\n' && (p->flags & PTY_ONLCR))
            ring_push(p->out, &p->out_head, &p->out_tail, &p->out_count, PTY_RING, '\r');
        ring_push(p->out, &p->out_head, &p->out_tail, &p->out_count, PTY_RING, s[i]);
        n++;
    }
    spin_unlock_irqrestore(&p->lock, fl);
    return n;
}

int pty_selftest(void) {
    int ok = 1;
    int p = pty_create(PTY_ICANON | PTY_ECHO | PTY_ONLCR);
    if (p < 0) {
        serial_puts("[14B] pty OOM\n");
        return -1;
    }
    /* Kanonik düzenleme: "ab" + backspace + "c" + enter -> "ac\n" */
    pty_slave_input(p, 'a');
    pty_slave_input(p, 'b');
    pty_slave_input(p, 0x08);
    pty_slave_input(p, 'c');
    {
        char tmp[PTY_LINE];
        if (pty_slave_read(p, tmp, sizeof(tmp)) != 0) ok = 0; /* satır yok henüz */
    }
    pty_slave_input(p, '\n');
    {
        char line[PTY_LINE];
        int n = pty_slave_read(p, line, sizeof(line));
        if (n != 3 || line[0] != 'a' || line[1] != 'c' || line[2] != '\n') ok = 0;
        /* Yankı tamponunda 'a' görülmeli */
        char echo[PTY_LINE];
        int m = pty_master_read(p, echo, sizeof(echo));
        int found_a = 0;
        for (int i = 0; i < m; i++) if (echo[i] == 'a') found_a = 1;
        if (!found_a) ok = 0;
    }
    /* Ctrl-U satırı siler */
    pty_slave_input(p, 'x');
    pty_slave_input(p, 'y');
    pty_slave_input(p, 0x15);
    pty_slave_input(p, 'z');
    pty_slave_input(p, '\n');
    {
        char line[PTY_LINE];
        int n = pty_slave_read(p, line, sizeof(line));
        if (n != 2 || line[0] != 'z' || line[1] != '\n') ok = 0;
    }
    /* ONLCR: yankı artığını boşalt, sonra master_write "q\n" -> "q\r\n" */
    {
        char drain[PTY_RING];
        pty_master_read(p, drain, sizeof(drain));
    }
    pty_master_write(p, "q\n", 2);
    {
        char out[8];
        int n = pty_master_read(p, out, sizeof(out));
        if (n != 3 || out[0] != 'q' || out[1] != '\r' || out[2] != '\n') ok = 0;
    }
    /* Ham mod: anında okunur */
    int r = pty_create(0);
    if (r < 0) ok = 0;
    else {
        pty_slave_input(r, 'z');
        char b;
        if (pty_slave_read(r, &b, 1) != 1 || b != 'z') ok = 0;
        pty_destroy(r);
    }
    /* Geçersiz id'ler */
    if (pty_slave_input(99, 'a') == 0) ok = 0;
    if (pty_slave_read(99, (char[4]){0}, 4) > 0) ok = 0;
    pty_destroy(p);
    if (ok) {
        serial_puts("[14B] pty ldisc/echo/onlcr [PASS]\n");
        vga_puts("[14B] pty ldisc/echo/onlcr [PASS]\n");
        return 0;
    }
    serial_puts("[14B] pty [FAIL]\n");
    vga_puts("[14B] pty [FAIL]\n");
    return -1;
}
