#include <drivers/inr.h>
#include <drivers/input.h>
#include <drivers/wm.h>
#include <drivers/compositor.h>
#include <drivers/fb.h>
#include <drivers/term.h>
#include <drivers/fm.h>
#include <drivers/clip.h>
#include <drivers/vt.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <process/task.h>
#include <string.h>
#include <stdint.h>

/* 18M: Input Router — hub -> pencere yönlendirme.
 *
 * Fare (hub IN_EV_MOUSE_MOVE/BTN ya da sentetik): imleç konumu, hit-test
 * ile click-to-focus, başlık çubuğundan sürükleme. Klavye (hub KEY_PRESS):
 * set-1 scancode -> ASCII decode, odaklı pencereye (terminal -> PTY).
 */

static int inr_on = 0;
static int inr_task_started = 0;
static int cur_x = 0, cur_y = 0;
static int scr_w = 1024, scr_h = 768;
static int prev_buttons = 0;
static int shift_down = 0;
static int ctrl_down = 0; /* 18P: pano kısayolları */
static int alt_down = 0;  /* 18Q: Alt+Fn VT geçişi */
static char paste_tmp[CLIP_MAX]; /* 18P: yığın yerine statik tampon */
static int drag_win = -1;
static int routed_keys = 0;
static int routed_clicks = 0;

/* --- set-1 scancode -> ASCII (harf/rakam/boşluk/enter/tab/backspace/esc) --- */
static char key_decode(uint8_t sc) {
    static const char rows[4][10] = {
        {'1','2','3','4','5','6','7','8','9','0'},
        {'Q','W','E','R','T','Y','U','I','O','P'},
        {'A','S','D','F','G','H','J','K','L',';'},
        {'Z','X','C','V','B','N','M',',','.','/'}
    };
    static const uint8_t codes[4][10] = {
        {0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B},
        {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19},
        {0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27},
        {0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35}
    };
    static const char shifted_digits[10] = {')','!','@','#','$','%','^','&','*','('};
    if (sc == 0x39) return ' ';
    if (sc == 0x1C) return '\n';
    if (sc == 0x0F) return '\t';
    if (sc == 0x0E) return '\b';
    if (sc == 0x01) return 27;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            if (codes[r][c] != sc) continue;
            char ch = rows[r][c];
            if (r == 0 && shift_down) return shifted_digits[c];
            if (r >= 1 && r <= 3) {
                if (ch >= 'A' && ch <= 'Z' && !shift_down) ch += 32;
                return ch;
            }
            return ch;
        }
    }
    return 0;
}

static void clamp_cursor(void) {
    if (cur_x < 0) cur_x = 0;
    if (cur_y < 0) cur_y = 0;
    if (cur_x >= scr_w) cur_x = scr_w - 1;
    if (cur_y >= scr_h) cur_y = scr_h - 1;
}

/* Fare akıcılığı: _nb çeşitleri sunum yapmaz; inr_poll tur sonunda
 * tek composite yeter (olay başına 3MB flip yerine). */
static void inr_mouse_move_nb(int dx, int dy) {
    cur_x += dx;
    cur_y += dy;
    clamp_cursor();
    comp_cursor_move(cur_x, cur_y);
    if (drag_win >= 0 && vt_gui_active()) wm_drag_to(cur_x, cur_y);
}

static void inr_mouse_button_nb(int buttons) {
    if (!vt_gui_active()) { prev_buttons = buttons; return; } /* 18Q: metin VT */
    int pressed = buttons & 1;
    int was = prev_buttons & 1;
    prev_buttons = buttons;
    if (pressed && !was) {
        /* Sol tuş basıldı: hit-test + odakla */
        int hit = wm_hit_test(cur_x, cur_y);
        if (hit >= 0) {
            int wx = 0, wy = 0, ww = 0, wh = 0;
            wm_get_rect(hit, &wx, &wy, &ww, &wh);
            (void)ww; (void)wh;
            wm_focus(hit);
            routed_clicks++;
            if (cur_y - wy < WM_TITLE_H) {
                wm_drag_start(hit, cur_x, cur_y);
                drag_win = hit;
            }
        }
    } else if (!pressed && was) {
        /* Bırakıldı: sürüklemeyi bitir */
        if (drag_win >= 0) {
            wm_drag_to(cur_x, cur_y);
            wm_drag_end();
            drag_win = -1;
        }
    }
}

void inr_mouse_move(int dx, int dy) {
    if (!inr_on) return;
    inr_mouse_move_nb(dx, dy);
    comp_composite();
}

void inr_mouse_button(int buttons) {
    if (!inr_on) return;
    inr_mouse_button_nb(buttons);
    comp_composite();
}

/* Kısıt-2: odaklı terminal varken klavye IRQ tamponu atlar; tuşu
 * hub üzerinden inr dağıtır. Salt okuma yapar (IRQ güvenli). */
int inr_claims_keyboard(void) {
    if (!inr_on) return 0;
    if (!vt_gui_active()) return 0; /* metin VT kendi yolunu kullanır */
    int f = wm_focused();
    return (f >= 0 && f == term_window_id() && term_ready()) ? 1 : 0;
}

int inr_key_event(uint8_t scancode, int pressed) {
    if (!inr_on) return 0;
    if (scancode == 0x2A || scancode == 0x36) { shift_down = pressed; return 0; }
    if (scancode == 0x1D) { ctrl_down = pressed; return 0; } /* 18P */
    if (scancode == 0x38) { alt_down = pressed; return 0; }  /* 18Q */
    /* 18Q: Alt+F1..F4 sanal terminal geçişi (hub üzerinden, canlı+sentetik) */
    if (pressed && alt_down && scancode >= 0x3B && scancode <= 0x3E) {
        vt_switch((int)(scancode - 0x3B) + VT_GUI);
        return 1;
    }
    if (!pressed) return 0;
    if (!vt_gui_active()) return 0; /* 18Q: metin VT'de GUI'ye tuş gitmez */
    /* 18P: Ctrl+C kes, Ctrl+X kes (kopyala), Ctrl+V yapıştır — odaklı terminal */
    if (ctrl_down && (scancode == 0x2E || scancode == 0x2F || scancode == 0x2D)) {
        int f = wm_focused();
        if (f < 0 || f != term_window_id() || !term_ready()) return 0;
        if (scancode == 0x2F) {
            int pn = clip_paste(paste_tmp, sizeof(paste_tmp));
            if (pn > 0 && term_input(paste_tmp) >= 0) { routed_keys++; return 1; }
            return 0;
        }
        {
            char lb[TERM_BUF_W];
            int ln = term_get_last_line(lb, sizeof(lb));
            if (ln >= 0 && clip_copy(lb) >= 0) { routed_keys++; return 1; }
            return 0;
        }
    }
    char c = key_decode(scancode);
    if (!c) return 0;
    int focus = wm_focused();
    if (focus < 0) return 0;
    if (focus == term_window_id() && term_ready()) {
        char tmp[2];
        tmp[0] = c; tmp[1] = 0;
        if (term_input(tmp) >= 0) {
            routed_keys++;
            return 1;
        }
    }
    return 0;
}

int inr_poll(void) {
    if (!inr_on) return 0;
    struct input_event ev;
    int n = 0, visual = 0;
    while (input_pop(&ev)) {
        switch (ev.type) {
        case IN_EV_MOUSE_MOVE:
            inr_mouse_move_nb(ev.x, ev.y);
            inr_mouse_button_nb(ev.code);
            visual = 1;
            break;
        case IN_EV_MOUSE_BTN:
            inr_mouse_button_nb(ev.code);
            visual = 1;
            break;
        case IN_EV_KEY_PRESS:
            inr_key_event(ev.code, 1);
            break;
        case IN_EV_KEY_RELEASE:
            inr_key_event(ev.code, 0);
            break;
        default:
            break;
        }
        n++;
    }
    if (visual) comp_composite(); /* tur başına tek sunum */
    return n;
}

static void inr_task(void) {
    for (;;) {
        int n = inr_poll();
        if (n <= 0) task_sleep(2); /* blokla: rotasyonu şişirme */
    }
}

int inr_init(void) {
    if (inr_on) return 0;
    if (!fb_active()) return -1;
    {
        uint32_t mw = 0, mh = 0, mb = 0;
        fb_get_mode(&mw, &mh, &mb);
        if (mw >= 640 && mh >= 480) { scr_w = (int)mw; scr_h = (int)mh; }
    }
    cur_x = scr_w / 2;
    cur_y = scr_h / 2;

    /* 18M: canlı masaüstü yerleşimi — pencereleri kademelendir */
    {
        int t = term_window_id();
        if (t >= 0) wm_move(t, 100, 60);
        int f = fm_window_id();
        if (f >= 0) wm_move(f, 440, 140);
    }
    comp_cursor_show(cur_x, cur_y);
    {
        int t = term_window_id();
        if (t >= 0) wm_focus(t);
    }
    comp_composite();
    inr_on = 1;
    serial_puts("[18M] input router hazır\n");
    return 0;
}

void inr_start_task(void) {
    if (!inr_on || inr_task_started) return;
    task_create(inr_task, "inrouter");
    inr_task_started = 1;
    serial_puts("[18M] inrouter task started\n");
}

int inr_cursor_pos(int* x, int* y) {
    if (x) *x = cur_x;
    if (y) *y = cur_y;
    return inr_on;
}

int inr_routed_keys(void) { return routed_keys; }
int inr_routed_clicks(void) { return routed_clicks; }

/* Absolut konumlandırma (selftest/sürücü için göreli hamle) */
static void inr_goto(int x, int y) {
    int cx = 0, cy = 0;
    inr_cursor_pos(&cx, &cy);
    inr_mouse_move(x - cx, y - cy);
}

int inr_selftest(void) {
    if (!inr_on) {
        serial_puts("[18M] input test atlandı\n");
        return -1;
    }
    int ok = 1;
    int T = term_window_id();
    int F = fm_window_id();
    if (T < 0 || F < 0) {
        serial_puts("[18M] pencere yok\n");
        return -1;
    }
    int tx = 0, ty = 0, tw = 0, th = 0;
    int fx = 0, fy = 0, fw = 0, fh = 0;
    if (wm_get_rect(T, &tx, &ty, &tw, &th) != 0) ok = 0;
    if (wm_get_rect(F, &fx, &fy, &fw, &fh) != 0) ok = 0;
    if (tw <= 0 || th <= 0 || fw <= 0 || fh <= 0) ok = 0;

    /* Sabit test yerleşimi (çakışmasız): terminal sol-üst, fm sağ */
    wm_move(T, 100, 60);
    wm_move(F, 440, 140);
    comp_composite();
    wm_get_rect(T, &tx, &ty, &tw, &th);
    wm_get_rect(F, &fx, &fy, &fw, &fh);

    /* 1) İmleç terminal gövdesinde -> hit terminal olmalı */
    int tbx = tx + tw / 2, tby = ty + WM_TITLE_H + (th - WM_TITLE_H) / 2;
    inr_goto(tbx, tby);
    if (wm_hit_test(tbx, tby) != T) ok = 0;
    /* Tıkla -> odak terminal */
    inr_mouse_button(1);
    inr_mouse_button(0);
    if (wm_focused() != T) ok = 0;

    /* 2) Başlık çubuğundan sürükle: pencere (50,40) kaymalı */
    inr_goto(tx + 20, ty + 5);
    inr_mouse_button(1);
    if (wm_focused() != T) ok = 0;
    inr_mouse_move(50, 40);
    {
        int nx = 0, ny = 0;
        wm_get_rect(T, &nx, &ny, 0, 0);
        if (nx != tx + 50 || ny != ty + 40) ok = 0;
    }
    inr_mouse_button(0);
    if (wm_dragging() >= 0) ok = 0;
    /* Bırakma sonrası hareket pencereyi oynatmamalı */
    inr_mouse_move(10, 10);
    {
        int nx = 0, ny = 0;
        wm_get_rect(T, &nx, &ny, 0, 0);
        if (nx != tx + 50 || ny != ty + 40) ok = 0;
    }

    /* 3) FM gövdesine tıkla -> odak fm olmalı */
    wm_get_rect(F, &fx, &fy, &fw, &fh);
    inr_goto(fx + fw / 2, fy + WM_TITLE_H + 20);
    if (wm_hit_test(fx + fw / 2, fy + WM_TITLE_H + 20) != F) ok = 0;
    inr_mouse_button(1);
    inr_mouse_button(0);
    if (wm_focused() != F) ok = 0;

    /* 4) Klavye yönlendirme: terminali odakla, A/B/enter gönder */
    inr_goto(tx + 50 + tw / 2, ty + 40 + th / 2);
    inr_mouse_button(1);
    inr_mouse_button(0);
    if (wm_focused() != T) ok = 0;
    {
        int ck0 = inr_routed_keys();
        int L0 = term_get_line_count();
        input_push(IN_EV_KEY_PRESS, 0x1E, 0, 0);   /* A */
        input_push(IN_EV_KEY_RELEASE, 0x1E, 0, 0);
        input_push(IN_EV_KEY_PRESS, 0x30, 0, 0);   /* B */
        input_push(IN_EV_KEY_RELEASE, 0x30, 0, 0);
        input_push(IN_EV_KEY_PRESS, 0x1C, 0, 0);   /* enter */
        input_push(IN_EV_KEY_RELEASE, 0x1C, 0, 0);
        int drained = inr_poll();
        if (drained < 6) ok = 0;
        if (inr_routed_keys() < ck0 + 3) ok = 0;
        if (term_get_line_count() <= L0) ok = 0;
    }

    /* 5) İmleç görünür: ok ucu beyaz piksel olmalı */
    {
        int cx = 0, cy = 0;
        inr_cursor_pos(&cx, &cy);
        uint32_t p = fb_getpixel_front((uint32_t)cx, (uint32_t)cy);
        if ((p & 0x00FFFFFFu) != 0x00FFFFFFu) ok = 0;
    }

    /* Yerleşimi geri al */
    wm_move(T, tx, ty);
    wm_move(F, fx, fy);
    comp_composite();

    if (ok) {
        serial_puts("[18M] input routing/focus/drag/keys [PASS]\n");
        vga_puts("[18M] input routing/focus/drag/keys [PASS]\n");
        return 0;
    }
    serial_puts("[18M] input routing [FAIL]\n");
    vga_puts("[18M] input routing [FAIL]\n");
    return -1;
}
