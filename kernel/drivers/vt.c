#include <drivers/vt.h>
#include <drivers/compositor.h>
#include <drivers/fb.h>
#include <drivers/font.h>
#include <drivers/inr.h>
#include <drivers/input.h>
#include <drivers/gfx.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <drivers/theme.h>
#include <drivers/wm.h>
#include <process/task.h>
#include <string.h>

/* 18Q: Sanal terminaller. VT1 = GUI (compositor), VT2-4 = 80x25 metin.
 * Metin VT doğrudan fb arka tamponuna çizilir; GUI katmanı bayrağı ile
 * compositor duraklatılır, dönüşte full_repaint yapılır. */

#define VT_TEXT_N 3  /* VT2, VT3, VT4 */
#define VSH_LINE_MAX 128

struct vt_screen {
    char cells[VT_ROWS][VT_COLS];
    uint8_t lens[VT_ROWS];
    int cx, cy; /* imleç konumu */
};

/* Kısıt-3 (vsh): VT kabuğu satır tamponları (IRQ biriktirir, task yürütür) */
static char vt_line[VT_TEXT_N][VSH_LINE_MAX];
static int vt_line_len[VT_TEXT_N];
static volatile int vt_line_ready[VT_TEXT_N];
static int vt_task_started = 0;

static int vt_on = 0;
static int vt_cur = VT_GUI;
static struct vt_screen vt_screens[VT_TEXT_N];
static struct gfx_surface vt_fb;

static int vt_idx(int vt) { return vt - 2; } /* VT2->0 */

static void vt_fb_init(void) {
    vt_fb.pixels = fb_back_buffer();
    uint32_t w = 1024, h = 768, bpp = 32;
    fb_get_mode(&w, &h, &bpp);
    if (w < 640 || h < 200) { w = 1024; h = 768; }
    vt_fb.w = w; vt_fb.h = h;
    vt_fb.pitch = fb_pitch_bytes();
    vt_fb.bypp = fb_bytes_per_pixel();
}

static void vt_scroll(struct vt_screen* s) {
    for (int r = 0; r < VT_ROWS - 1; r++) {
        for (int c = 0; c < VT_COLS; c++) s->cells[r][c] = s->cells[r + 1][c];
        s->lens[r] = s->lens[r + 1];
    }
    for (int c = 0; c < VT_COLS; c++) s->cells[VT_ROWS - 1][c] = ' ';
    s->lens[VT_ROWS - 1] = 0;
}

static void vt_newline(struct vt_screen* s) {
    s->cx = 0;
    s->cy++;
    if (s->cy >= VT_ROWS) { vt_scroll(s); s->cy = VT_ROWS - 1; }
}

static void vt_putc_locked(struct vt_screen* s, char c) {
    if (c == '\n') { vt_newline(s); return; }
    if (c == '\r') { s->cx = 0; return; }
    if (c == '\b') {
        if (s->cx > 0) {
            s->cx--;
            s->cells[s->cy][s->cx] = ' ';
            if (s->lens[s->cy] > (uint8_t)s->cx) s->lens[s->cy] = (uint8_t)s->cx;
        }
        return;
    }
    if (c == '\t') {
        do { vt_putc_locked(s, ' '); } while (s->cx % 8 != 0);
        return;
    }
    if ((unsigned char)c < 32 || (unsigned char)c == 127) return;
    if (s->cx >= VT_COLS) vt_newline(s);
    s->cells[s->cy][s->cx] = c;
    s->cx++;
    if (s->cx > VT_COLS) s->cx = VT_COLS;
    if (s->lens[s->cy] < (uint8_t)s->cx && s->cx <= VT_COLS)
        s->lens[s->cy] = (uint8_t)(s->cx <= VT_COLS ? s->cx : VT_COLS);
}

/* Aktif metin VT'yi framebuffer'a çiz + flip */
static void vt_render(void) {
    if (!vt_on || vt_cur == VT_GUI) return;
    struct vt_screen* s = &vt_screens[vt_idx(vt_cur)];
    gfx_fill_rect(&vt_fb, 0, 0, (int)vt_fb.w, (int)vt_fb.h, 0xFF000000u);
    char line[VT_COLS + 1];
    for (int r = 0; r < VT_ROWS; r++) {
        int n = s->lens[r];
        if (n > VT_COLS) n = VT_COLS;
        for (int c = 0; c < n; c++) line[c] = s->cells[r][c];
        line[n] = 0;
        gfx_text_utf8(&vt_fb, 0, r * 8, line, 0xFFCCCCCCu);
    }
    /* Blok imleç */
    gfx_fill_rect(&vt_fb, s->cx * 8, s->cy * 8, 8, 8, 0xFFCCCCCCu);
    fb_damage_all(); /* doğrudan arka-tampon yazımı: tamamını kirlet */
    fb_flip();
}

int vt_init(void) {
    if (vt_on) return 0;
    if (!fb_active()) return -1;
    vt_fb_init();
    memset(vt_screens, 0, sizeof(vt_screens));
    for (int i = 0; i < VT_TEXT_N; i++)
        for (int r = 0; r < VT_ROWS; r++)
            for (int c = 0; c < VT_COLS; c++)
                vt_screens[i].cells[r][c] = ' ';
    vt_cur = VT_GUI;
    comp_set_gui_active(1);
    vt_on = 1;
    serial_puts("[18Q] sanal terminaller hazır\n");
    return 0;
}

int vt_gui_active(void) {
    if (!vt_on) return 1;
    return vt_cur == VT_GUI;
}

int vt_active(void) { return vt_cur; }

int vt_switch(int vt) {
    if (vt < VT_MIN || vt > VT_MAX) return -1;
    if (!vt_on) return -1;
    if (vt == vt_cur) return 0;
    vt_cur = vt;
    if (vt == VT_GUI) {
        comp_set_gui_active(1);
        comp_cursor_show(0, 0);
        {
            int cx = 0, cy = 0;
            if (inr_cursor_pos(&cx, &cy)) comp_cursor_move(cx, cy);
        }
        comp_full_repaint();
    } else {
        comp_cursor_hide();
        comp_set_gui_active(0);
        vt_render();
    }
    return 0;
}

int vt_putc(int vt, char c) {
    if (!vt_on) return -1;
    if (vt == VT_GUI) return -1;
    if (vt < VT_MIN || vt > VT_MAX) return -1;
    vt_putc_locked(&vt_screens[vt_idx(vt)], c);
    if (vt == vt_cur) vt_render();
    return 0;
}

int vt_write(int vt, const char* s) {
    if (!vt_on || !s) return -1;
    if (vt == VT_GUI) return -1;
    if (vt < VT_MIN || vt > VT_MAX) return -1;
    struct vt_screen* sc = &vt_screens[vt_idx(vt)];
    int n = 0;
    while (*s) { vt_putc_locked(sc, *s); s++; n++; }
    if (vt == vt_cur) vt_render();
    return n;
}

/* count-sınırlı vt_write: sys_write arayüzü için. Render YAPMAZ — ekran,
 * vt_task döngüsünde throttled olarak güncellenir (yüksek frekanslı sys_write
 * çıktısını her satırdan fb'a çizmek testleri boğardı). */
int vt_write_n(int vt, const char* s, int len) {
    if (!vt_on || !s || len <= 0) return -1;
    if (vt == VT_GUI) return -1;
    if (vt < VT_MIN || vt > VT_MAX) return -1;
    struct vt_screen* sc = &vt_screens[vt_idx(vt)];
    int n = 0;
    while (n < len && s[n]) { vt_putc_locked(sc, s[n]); n++; }
    return n;
}

void vt_input_char(char c) {
    if (!vt_on || vt_cur == VT_GUI) return;
    int i = vt_idx(vt_cur);
    vt_putc_locked(&vt_screens[i], c);
    /* Kabuk satırı biriktir (ekran yankısı yukarıda yapıldı) */
    if (c == '\n') {
        vt_line_ready[i] = 1;
    } else if (c == '\b') {
        if (vt_line_len[i] > 0) {
            vt_line_len[i]--;
            vt_line[i][vt_line_len[i]] = 0;
        }
    } else if (c == '\t') {
        if (vt_line_len[i] < VSH_LINE_MAX - 1) {
            vt_line[i][vt_line_len[i]++] = c;
            vt_line[i][vt_line_len[i]] = 0;
        }
    } else if ((unsigned char)c >= 32 && (unsigned char)c != 127) {
        if (vt_line_len[i] < VSH_LINE_MAX - 1) {
            vt_line[i][vt_line_len[i]++] = c;
            vt_line[i][vt_line_len[i]] = 0;
        }
    }
    vt_render();
}

int vt_clear(int vt) {
    if (!vt_on || vt == VT_GUI || vt < VT_MIN || vt > VT_MAX) return -1;
    int i = vt_idx(vt);
    for (int r = 0; r < VT_ROWS; r++) {
        for (int c = 0; c < VT_COLS; c++) vt_screens[i].cells[r][c] = ' ';
        vt_screens[i].lens[r] = 0;
    }
    vt_screens[i].cx = 0;
    vt_screens[i].cy = 0;
    vt_line_len[i] = 0;
    vt_line[i][0] = 0;
    vt_line_ready[i] = 0;
    if (vt == vt_cur) vt_render();
    return 0;
}

/* --- vsh: VT kabuğu (yerleşik komutlar) --- */
static void vt_prompt(int vt) {
    char p[8];
    p[0] = 'v'; p[1] = 't';
    p[2] = (char)('0' + vt);
    p[3] = '>'; p[4] = ' '; p[5] = 0;
    vt_write(vt, p);
}

static void vt_print_dec(int vt, int v) {
    char rev[12];
    int rl = 0;
    int neg = 0;
    if (v < 0) { neg = 1; v = -v; }
    if (v == 0) rev[rl++] = '0';
    while (v > 0 && rl < 11) { rev[rl++] = (char)('0' + (v % 10)); v /= 10; }
    if (neg && rl < 11) rev[rl++] = '-';
    char out[12];
    int o = 0;
    while (rl > 0) out[o++] = rev[--rl];
    out[o] = 0;
    vt_write(vt, out);
}

/* Komut + argüman ayrıştırma (boşluk/tab). argv bölgesi line kopyası üstünde. */
static void vsh_exec(int vt, const char* line) {
    char buf[VSH_LINE_MAX];
    int n = 0;
    while (line[n] && n < VSH_LINE_MAX - 1) { buf[n] = line[n]; n++; }
    buf[n] = 0;
    /* Baştaki ayraçları atla */
    int s = 0;
    while (buf[s] == ' ' || buf[s] == '\t') s++;
    /* Komut sonunu bul */
    int e = s;
    while (buf[e] && buf[e] != ' ' && buf[e] != '\t') e++;
    char save = buf[e];
    buf[e] = 0;
    const char* cmd = buf + s;
    const char* arg = buf + e + (save ? 1 : 0);
    while (*arg == ' ' || *arg == '\t') arg++;

    if (cmd[0] == 0) { vt_prompt(vt); return; }
    if (strcmp(cmd, "help") == 0) {
        vt_write(vt, "komutlar: help echo clear theme hidpi ver windows\n");
    } else if (strcmp(cmd, "echo") == 0) {
        vt_write(vt, arg);
        vt_write(vt, "\n");
    } else if (strcmp(cmd, "clear") == 0) {
        vt_clear(vt);
    } else if (strcmp(cmd, "ver") == 0) {
        vt_write(vt, "charOS vsh 1.0\n");
    } else if (strcmp(cmd, "theme") == 0) {
        if (strcmp(arg, "dark") == 0) { theme_set(THEME_DARK); theme_apply(); }
        else if (strcmp(arg, "light") == 0) { theme_set(THEME_LIGHT); theme_apply(); }
        vt_write(vt, theme_get() == THEME_LIGHT ? "tema: light\n" : "tema: dark\n");
    } else if (strcmp(cmd, "hidpi") == 0) {
        if (strcmp(arg, "1") == 0) theme_set_hidpi(1);
        else if (strcmp(arg, "2") == 0) theme_set_hidpi(2);
        vt_write(vt, theme_ui_scale() == 2 ? "hidpi: 2x\n" : "hidpi: 1x\n");
    } else if (strcmp(cmd, "windows") == 0) {
        int ids[16];
        int m = wm_list_ids(ids, 16);
        for (int k = 0; k < m; k++) {
            vt_print_dec(vt, ids[k]);
            vt_write(vt, " ");
            const char* t = wm_get_title(ids[k]);
            vt_write(vt, t ? t : "?");
            vt_write(vt, "\n");
        }
    } else {
        vt_write(vt, "? bilinmeyen komut (help)\n");
    }
    vt_prompt(vt);
}

/* Hazır satır varsa yürüt (task + selftest ortak yolu) */
static void vt_run_pending(int vt) {
    int i = vt_idx(vt);
    if (!vt_line_ready[i]) return;
    vt_line_ready[i] = 0;
    char line[VSH_LINE_MAX];
    int n = vt_line_len[i];
    if (n > VSH_LINE_MAX - 1) n = VSH_LINE_MAX - 1;
    for (int k = 0; k < n; k++) line[k] = vt_line[i][k];
    line[n] = 0;
    vt_line_len[i] = 0;
    vt_line[i][0] = 0;
    vsh_exec(vt, line);
}

static void vt_task(void) {
    int grow = 0;
    for (;;) {
        int busy = 0;
        for (int vt = VT_MIN + 1; vt <= VT_MAX; vt++) {
            if (vt_line_ready[vt_idx(vt)]) { vt_run_pending(vt); busy = 1; }
        }
        /* 19K: konsolu seyrek tazele. sys_write render'sız yazar; ekran burada
         * ~500ms'de bir güncellenir. Sık render, high-frequency sys_write'la
         * birleşince arka plan job zamanlamasını (19G sh_run_script timeout'u)
         * bozuyordu; 25x2=50tick aralığıyla ekran canlı kalır, race bozulmaz. */
        grow++;
        if (vt_on && vt_cur != VT_GUI && (grow % 25) == 0) vt_render();
        if (!busy) task_sleep(2); /* blokla: rotasyonu şişirme */
    }
}

void vt_start_task(void) {
    if (!vt_on || vt_task_started) return;
    task_create(vt_task, "vtsh");
    vt_task_started = 1;
    serial_puts("[18Q] vtsh task started\n");
}

int vt_get_row(int vt, int row, char* out, int max) {
    if (!out || max <= 0) return -1;
    out[0] = 0;
    if (!vt_on || vt == VT_GUI || vt < VT_MIN || vt > VT_MAX) return -1;
    if (row < 0 || row >= VT_ROWS) return -1;
    struct vt_screen* s = &vt_screens[vt_idx(vt)];
    int n = s->lens[row];
    if (n > VT_COLS) n = VT_COLS;
    if (n > max - 1) n = max - 1;
    for (int i = 0; i < n; i++) out[i] = s->cells[row][i];
    out[n] = 0;
    return n;
}

/* VT tamponunda kelime tara (selftest yardımcısı) */
static int vt_row_contains(int vt, const char* word) {
    char row[VT_COLS + 1];
    for (int r = 0; r < VT_ROWS; r++) {
        if (vt_get_row(vt, r, row, sizeof(row)) < 0) return 0;
        for (int i = 0; row[i]; i++) {
            int k = 0;
            while (word[k] && row[i + k] == word[k]) k++;
            if (word[k] == 0) return 1;
        }
    }
    return 0;
}

static void vt_feed(const char* keys) {
    for (const char* k = keys; *k; k++) vt_input_char(*k);
}

int vt_selftest(void) {
    if (!vt_on) {
        serial_puts("[18Q] vt test atlandı\n");
        return -1;
    }
    int ok = 1;
    if (vt_active() != VT_GUI) ok = 0;
    if (vt_switch(9) == 0) ok = 0;      /* geçersiz reddedilmeli */
    if (vt_active() != VT_GUI) ok = 0;

    /* VT2'ye yaz, framebuffer'da glif pikseli görünmeli */
    if (vt_switch(2) != 0) ok = 0;
    if (!vt_gui_active()) { /* metin VT aktif olmalı */ }
    else ok = 0;
    vt_write(2, "HELLO-VT2");
    {
        /* 'H' (0,0): 1. satır 0x88 -> (0,1) ve (7,1) dolu */
        uint32_t p = fb_getpixel_front(0, 1);
        if ((p & 0x00FFFFFFu) == 0) ok = 0;
    }

    /* VT3 bağımsızlığı: farklı içerik, geri dönünce VT2 korunur */
    if (vt_switch(3) != 0) ok = 0;
    vt_write(3, "XYZ");
    {
        char row[VT_COLS + 1];
        if (vt_get_row(3, 0, row, sizeof(row)) < 0) ok = 0;
        else if (row[0] != 'X' || row[1] != 'Y' || row[2] != 'Z') ok = 0;
    }
    if (vt_switch(2) != 0) ok = 0;
    {
        char row[VT_COLS + 1];
        if (vt_get_row(2, 0, row, sizeof(row)) < 0) ok = 0;
        else {
            const char* want = "HELLO-VT2";
            for (int i = 0; want[i]; i++)
                if (row[i] != want[i]) { ok = 0; break; }
        }
        uint32_t p = fb_getpixel_front(0, 1);
        if ((p & 0x00FFFFFFu) == 0) ok = 0;
    }

    /* Giriş yankısı: aktif VT2'ye karakter */
    vt_input_char('Q');
    {
        char row[VT_COLS + 1];
        if (vt_get_row(2, 0, row, sizeof(row)) < 0) ok = 0;
        else if (row[9] != 'Q') ok = 0; /* "HELLO-VT2" 9 harf, sonra Q */
    }

    /* GUI'ye dönüş: compositor tazelenir, imleç geri gelir */
    if (vt_switch(VT_GUI) != 0) ok = 0;
    if (!vt_gui_active() || vt_active() != VT_GUI) ok = 0;
    {
        int cx = 0, cy = 0;
        inr_cursor_pos(&cx, &cy);
        uint32_t p = fb_getpixel_front((uint32_t)cx, (uint32_t)cy);
        if ((p & 0x00FFFFFFu) != 0x00FFFFFFu) ok = 0;
    }

    /* Alt+Fn kısayolu hub üzerinden: Alt+F3 -> VT3, Alt+F1 -> GUI */
    input_push(IN_EV_KEY_PRESS, 0x38, 0, 0);   /* Alt bas */
    input_push(IN_EV_KEY_PRESS, 0x3D, 0, 0);   /* F3 bas */
    input_push(IN_EV_KEY_RELEASE, 0x3D, 0, 0);
    input_push(IN_EV_KEY_RELEASE, 0x38, 0, 0); /* Alt bırak */
    inr_poll(); /* arka plan görevi de tüketebilir; sonuç aynı olmalı */
    {
        int spins = 0;
        while (vt_active() != 3 && spins < 1000) { inr_poll(); spins++; }
        if (vt_active() != 3) ok = 0;
    }
    input_push(IN_EV_KEY_PRESS, 0x38, 0, 0);
    input_push(IN_EV_KEY_PRESS, 0x3B, 0, 0);   /* F1 bas */
    input_push(IN_EV_KEY_RELEASE, 0x3B, 0, 0);
    input_push(IN_EV_KEY_RELEASE, 0x38, 0, 0);
    inr_poll();
    {
        int spins = 0;
        while (vt_active() != VT_GUI && spins < 1000) { inr_poll(); spins++; }
        if (vt_active() != VT_GUI) ok = 0;
    }

    /* VT kabuğu (vsh): VT4'te satır yürütme, task yokken senkron yol */
    if (vt_switch(4) != 0) ok = 0;
    else {
        vt_feed("echo VSH-OK\n");
        if (!vt_line_ready[vt_idx(4)]) ok = 0;
        vt_run_pending(4);
        if (vt_line_ready[vt_idx(4)]) ok = 0;
        if (!vt_row_contains(4, "VSH-OK")) ok = 0;
        vt_feed("bogus-cmd-zz\n");
        vt_run_pending(4);
        if (!vt_row_contains(4, "bilinmeyen")) ok = 0;
        vt_feed("theme light\n");
        vt_run_pending(4);
        if (theme_get() != THEME_LIGHT) ok = 0;
        vt_feed("theme dark\n");
        vt_run_pending(4);
        if (theme_get() != THEME_DARK) ok = 0;
        vt_feed("windows\n");
        vt_run_pending(4);
        if (!vt_row_contains(4, "TERMINAL")) ok = 0;
        vt_feed("clear\n");
        vt_run_pending(4);
        {
            char row[VT_COLS + 1];
            if (vt_get_row(4, 0, row, sizeof(row)) < 0) ok = 0;
            else if (strncmp(row, "vt4> ", 5) != 0) ok = 0;
        }
    }
    if (vt_switch(VT_GUI) != 0) ok = 0;
    if (!vt_gui_active() || vt_active() != VT_GUI) ok = 0;

    if (ok) {
        serial_puts("[18Q] multi-VT switch/isolate/input [PASS]\n");
        vga_puts("[18Q] multi-VT switch/isolate/input [PASS]\n");
        return 0;
    }
    serial_puts("[18Q] multi-VT [FAIL]\n");
    vga_puts("[18Q] multi-VT [FAIL]\n");
    return -1;
}
