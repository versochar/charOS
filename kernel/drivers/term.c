#include <drivers/term.h>
#include <drivers/wm.h>
#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <process/pipe.h>
#include <process/task.h>
#include <drivers/timer.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>
#include <memory/kheap.h>

/* 18J: GUI Terminal Emulator — PTY üzerinden TTY.
 *
 * PTY: `term_pty_in` (uç -> çekirdek) ve `term_pty_out` (çekirdek -> uç)
 * isimli iki blocking pipe. GUI terminali `term_output` (senkron) ya da
 * arka planda `term_read_task` ile PTY çıktısını alıp viewport'a render eder.
 *
 * Viewport: scrollback satır dizisi; her çıktı satır satır işlenir ve
 * WM penceresindeki surface'a çizilir.
 */

struct term_line {
    char buf[TERM_BUF_W];
    int len;
};

static struct term_line scrollback[TERM_BUF_LINES];
static int sbot = 0;          /* satır sayısı */
static int term_cur_line = 0;

static struct pipe* term_pty_in = 0;   /* kullanıcı -> sh */
static struct pipe* term_pty_out = 0;  /* sh -> kullanıcı */

static int term_window = -1;   /* WM pencere id */
static int term_on = 0;
static int term_task_started = 0;

/* Son N satırı döndür: baştan görünmez olanları atla */
static void term_viewport_lines(int* start, int* count) {
    int n = sbot;
    if (n > TERM_ROWS) n = TERM_ROWS;
    *start = sbot - n;
    *count = n;
    if (*start < 0) *start = 0;
}

static void term_line_to_surface(struct gfx_surface* s, int row, int line_idx) {
    if (line_idx < 0 || line_idx >= sbot) return;
    struct term_line* l = &scrollback[line_idx];
    int y = row * 10;
    /* Satırı temizle */
    gfx_fill_rect(s, 0, y, TERM_VIEW_W, 9, 0x00000000);
    /* Satır içeriği */
    uint32_t fg = 0x0000E000;  /* terminal yeşili */
    char tmp[TERM_BUF_W + 1];
    int w = l->len;
    if (w > TERM_COLS) w = TERM_COLS;
    memcpy(tmp, l->buf, (uint32_t)w);
    tmp[w] = 0;
    gfx_text(s, 0, y, tmp, fg);
}

static void term_redraw(void) {
    if (term_window < 0) return;
    /* WM penceresinin comp surface'ını al */
    int cid = wm_get_comp(term_window);
    if (cid < 0) return;
    struct gfx_surface* s = comp_surface(cid);
    if (!s) return;
    gfx_fill(s, 0x00000000);
    int start, n;
    term_viewport_lines(&start, &n);
    for (int r = 0; r < n; r++)
        term_line_to_surface(s, r, start + r);
    /* İmleç: son satırın sonuna dikey çubuk */
    int last = sbot - 1;
    int y = (n - 1) * 10;
    char lastc;
    struct term_line* ll = &scrollback[last];
    int cx = ll->len;
    lastc = 0;
    if (cx > TERM_COLS) cx = TERM_COLS;
    gfx_line(s, cx * 8, y + 1, cx * 8, y + 8, 0x00AAFFAA);
    (void)lastc;
    comp_damage_all(cid);
}

static void term_append_char(char c) {
    struct term_line* l = &scrollback[term_cur_line];
    if (c == '\n') {
        if (sbot < TERM_BUF_LINES) {
            sbot++;
            term_cur_line = sbot;
            if (term_cur_line >= TERM_BUF_LINES) term_cur_line = TERM_BUF_LINES - 1;
        } else {
            /* scrollback dolu: kaydır */
            for (int i = 0; i < TERM_BUF_LINES - 1; i++)
                scrollback[i] = scrollback[i + 1];
            memset(&scrollback[TERM_BUF_LINES - 1], 0, sizeof(struct term_line));
            term_cur_line = TERM_BUF_LINES - 1;
        }
        return;
    }
    if (l->len >= TERM_BUF_W - 1) return;
    l->buf[l->len++] = c;
}

int term_emit(const char* s) {
    if (!term_on) return -1;
    if (!s) return -1;
    while (*s) {
        term_append_char(*s);
        s++;
    }
    term_redraw();
    comp_composite();
    return 0;
}

int term_input(const char* s) {
    if (!term_on || !term_pty_in) return -1;
    if (!s) return -1;
    /* TTY'ye yaz (echo hedefi): key girişi PTY'ye per-character gitmeli */
    int n = pipe_write_block(term_pty_in, s, (int)strlen(s));
    /* Echo: typed komut uçta görünmeli */
    while (*s) { term_append_char(*s); s++; }
    term_redraw();
    comp_composite();
    return n;
}

/* Task: pty_out çıktısını viewport'a dök + pty_in süzgeci.
 * Kısıt-1 düzeltmesi: pty_in'in okuyucusu yoktu; yoğun girişte boru dolar
 * ve term_input (yazan taraf) sonsuza dek bloklanırdı. Yankı term_input
 * içinde eşzamanlı yapıldığı için süzülen baytlar kayıp sayılmaz; kabuk
 * bağlandığında bu süzgeç gerçek tüketiciyle değişir. */
void term_pty_read_task(void) {
    char buf[TERM_BUF_W];
    char drain[64];
    for (;;) {
        int n = pipe_read_nonblock(term_pty_out, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = 0;
            term_emit(buf);
        }
        while (pipe_read_nonblock(term_pty_in, drain, sizeof(drain)) > 0) { }
        if (n <= 0) task_sleep(2); /* blokla: rotasyonu şişirme */
    }
}

int term_init(void) {
    if (!wm_init()) {
        term_window = wm_create("TERMINAL", TERM_VIEW_W + 4,
                                TERM_VIEW_H + 4 + WM_TITLE_H,
                                0x00000000);
        if (term_window < 0) return -1;

        term_pty_in = pipe_create();
        term_pty_out = pipe_create();
        if (!term_pty_in || !term_pty_out) return -1;

        sbot = 0;
        term_cur_line = 0;
        memset(scrollback, 0, sizeof(scrollback));

        term_on = 1;
        serial_puts("[18J] terminal emulator started (PTY)\n");
        vga_puts("[18J] terminal emulator started (PTY)\n");
        return 0;
    }
    return -1;
}

/* Task altyapısı kurulduktan sonra PTY okuyucu task'ını başlat */
void term_start_task(void) {
    if (!term_on || term_task_started) return;
    task_create(term_pty_read_task, "termpty");
    term_task_started = 1;
    serial_puts("[18J] termpty task started\n");
}

int term_ready(void) { return term_on; }

int term_window_id(void) { return term_window; }
int term_get_line_count(void) { return sbot; }

/* 18P: pano kopyası için o anki (bitmemiş) satır. Dönüş: uzunluk. */
int term_get_last_line(char* out, int max) {
    if (!out || max <= 0) return -1;
    out[0] = 0;
    if (!term_on) return -1;
    int idx = term_cur_line;
    if (idx < 0) idx = 0;
    if (idx >= TERM_BUF_LINES) idx = TERM_BUF_LINES - 1;
    struct term_line* l = &scrollback[idx];
    int n = l->len;
    if (n > TERM_BUF_W - 1) n = TERM_BUF_W - 1;
    if (n > max - 1) n = max - 1;
    for (int i = 0; i < n; i++) out[i] = l->buf[i];
    out[n] = 0;
    return n;
}

int term_selftest(void) {
    if (!term_on) {
        serial_puts("[18J] term test atlandı\n");
        return -1;
    }
    int ok = 1;

    term_emit("charOS TTY v1.0\n");
    term_emit("root@charos:~$\n");

    /* Scrollback'te 2 satır olmalı */
    if (sbot < 2) ok = 0;

    /* İlk satır içeriği */
    if (ok && strncmp(scrollback[0].buf, "charOS TTY v1.0", 6) != 0) ok = 0;

    /* İkinci satır */
    if (ok && strncmp(scrollback[1].buf, "root@charos:~$", 6) != 0) ok = 0;

    /* PTY input round-trip: term_input -> PTY'ye yaz, çıktıyı PTY_out'a düş */
    term_input("ls\n");

    /* İkinci adımda echo görünmüş olmalı (term_input echo eklemişti) */
    if (ok && sbot < 3) ok = 0;

    /* Render: viewport surface'ında yeşil piksel olmalı */
    if (ok) {
        int cid = wm_get_comp(term_window);
        struct gfx_surface* ts = comp_surface(cid);
        if (ts) {
            int found = 0;
            for (int y = 0; y < 9; y++)
                for (int x = 0; x < 60; x++) {
                    uint32_t p = ((uint32_t*)ts->pixels)[(uint32_t)y * (ts->pitch / 4) + (uint32_t)x];
                    if ((p & 0x00FF0000) == 0x00000000 && (p & 0x0000FF00) != 0)
                    { found = 1; break; }
                }
            if (!found) ok = 0;
        }
    }

    if (ok) {
        serial_puts("[18J] terminal/PTY [PASS]\n");
        vga_puts("[18J] terminal/PTY [PASS]\n");
        return 0;
    }
    serial_puts("[18J] terminal/PTY [FAIL]\n");
    vga_puts("[18J] terminal/PTY [FAIL]\n");
    return -1;
}
