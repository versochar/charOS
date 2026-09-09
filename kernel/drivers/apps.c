#include <drivers/apps.h>
#include <drivers/wm.h>
#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <drivers/font.h>
#include <drivers/widget.h>
#include <drivers/menu.h>
#include <drivers/shell.h>
#include <drivers/term.h>
#include <drivers/fm.h>
#include <drivers/theme.h>
#include <fs/chfs.h>
#include <fs/vfs.h>
#include <drivers/fb.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>
#include <memory/kheap.h>

/* 18S: Örnek uygulamalar (editör + görüntüleyici + ayarlar). */

/* ---------- Metin editörü ---------- */
#define EDIT_ROWS 16
#define EDIT_COLS 64
#define EDIT_BG   0x00101010u
#define EDIT_FG   0x00E8E8E8u

static int edit_win = -1;
static char edit_buf[EDIT_ROWS][EDIT_COLS];
static int edit_n = 0;
static int edit_dirty_flag = 0;
static char edit_path[FS_MAX_PATH];
static int apps_on = 0;

int edit_window_id(void) { return edit_win; }
int edit_nlines(void) { return edit_n; }
int edit_dirty(void) { return edit_dirty_flag; }

int edit_open(const char* path) {
    if (!path || !path[0]) return -1;
    strncpy(edit_path, path, FS_MAX_PATH - 1);
    edit_path[FS_MAX_PATH - 1] = 0;
    memset(edit_buf, 0, sizeof(edit_buf));
    edit_n = 0;
    edit_dirty_flag = 0;
    static char tmp[EDIT_ROWS * (EDIT_COLS + 1)];
    int n = chfs_read_file(path, tmp, sizeof(tmp) - 1);
    if (n < 0) return 0; /* yoksa boş başla */
    tmp[n] = 0;
    int r = 0, c = 0;
    for (int i = 0; i < n && r < EDIT_ROWS; i++) {
        if (tmp[i] == '\n') { r++; c = 0; continue; }
        if (tmp[i] == '\r') continue;
        if (c < EDIT_COLS - 1) edit_buf[r][c++] = tmp[i];
    }
    edit_n = r + (c > 0 ? 1 : 0);
    if (edit_n > EDIT_ROWS) edit_n = EDIT_ROWS;
    if (n > 0 && edit_n == 0) edit_n = 1;
    return 0;
}

int edit_set_line(int row, const char* text) {
    if (row < 0 || row >= EDIT_ROWS || !text) return -1;
    int i = 0;
    while (i < EDIT_COLS - 1 && text[i]) { edit_buf[row][i] = text[i]; i++; }
    edit_buf[row][i] = 0;
    if (row >= edit_n) edit_n = row + 1;
    edit_dirty_flag = 1;
    return 0;
}

int edit_get_line(int row, char* out, int max) {
    if (!out || max <= 0) return -1;
    out[0] = 0;
    if (row < 0 || row >= EDIT_ROWS) return -1;
    int i = 0;
    while (i < max - 1 && edit_buf[row][i]) { out[i] = edit_buf[row][i]; i++; }
    out[i] = 0;
    return 0;
}

int edit_save(void) {
    if (!edit_path[0]) return -1;
    static char tmp[EDIT_ROWS * (EDIT_COLS + 1)];
    int o = 0;
    for (int r = 0; r < edit_n && r < EDIT_ROWS; r++) {
        int i = 0;
        while (edit_buf[r][i] && o < (int)sizeof(tmp) - 2) tmp[o++] = edit_buf[r][i++];
        if (o < (int)sizeof(tmp) - 1) tmp[o++] = '\n';
    }
    int w = chfs_write_file(edit_path, tmp, o);
    if (w >= 0) edit_dirty_flag = 0;
    return w;
}

void edit_render(void) {
    if (edit_win < 0) return;
    int cid = wm_get_comp(edit_win);
    struct gfx_surface* s = comp_surface(cid);
    if (!s) return;
    gfx_fill(s, EDIT_BG);
    char status[80];
    strcpy(status, edit_path[0] ? edit_path : "(BOS)");
    strcat(status, " L:");
    {
        char nb[8];
        int v = edit_n, d = 0;
        if (v == 0) nb[d++] = '0';
        else {
            char rev[8];
            int rl = 0;
            while (v > 0 && rl < 7) { rev[rl++] = '0' + (v % 10); v /= 10; }
            while (rl > 0) nb[d++] = rev[--rl];
        }
        nb[d] = 0;
        strcat(status, nb);
    }
    if (edit_dirty_flag) strcat(status, " *");
    gfx_text_utf8(s, 4, 2, status, 0x00AAAAFFu);
    for (int r = 0; r < edit_n && r < EDIT_ROWS; r++) {
        char line[EDIT_COLS + 1];
        int i = 0;
        while (i < EDIT_COLS && edit_buf[r][i]) { line[i] = edit_buf[r][i]; i++; }
        line[i] = 0;
        gfx_text_utf8(s, 4, 12 + r * 8, line, EDIT_FG);
    }
    comp_damage_all(cid);
}

/* ---------- Görüntüleyici ---------- */
#define VIEW_SRC 64
#define VIEW_BG  0x00080808u

static int view_win = -1;
static int view_z = 1;
static uint32_t view_src[VIEW_SRC * VIEW_SRC];
static int view_src_ready = 0;

int view_window_id(void) { return view_win; }
int view_zoom(void) { return view_z; }

static void view_gen(void) {
    for (int y = 0; y < VIEW_SRC; y++) {
        for (int x = 0; x < VIEW_SRC; x++) {
            uint32_t r = (uint32_t)((x * 4) & 0xFF);
            uint32_t g = (uint32_t)((y * 4) & 0xFF);
            uint32_t b = (uint32_t)(((x + y) * 2) & 0xFF);
            if (((x / 8) + (y / 8)) % 2 == 0) {
                r = (r + 64) & 0xFF; g = (g + 64) & 0xFF; b = (b + 64) & 0xFF;
            }
            if (x == y) { r = 255; g = 255; b = 255; }        /* köşegen */
            if (x >= 8 && x < 56 && (y == 8 || y == 55)) { r = 0; g = 255; b = 0; }
            if (y >= 8 && y < 56 && (x == 8 || x == 55)) { r = 0; g = 255; b = 0; }
            view_src[(uint32_t)y * VIEW_SRC + (uint32_t)x] = (r << 16) | (g << 8) | b;
        }
    }
    view_src_ready = 1;
}

void view_render(void) {
    if (view_win < 0) return;
    int cid = wm_get_comp(view_win);
    struct gfx_surface* s = comp_surface(cid);
    if (!s) return;
    if (!view_src_ready) view_gen();
    gfx_fill(s, VIEW_BG);
    gfx_text_utf8(s, 4, 2, view_z == 2 ? "DEMO 64X64 Z2" : "DEMO 64X64 Z1",
                  0x00BBBBBBu);
    int ox = 8, oy = 24;
    int sw = (int)s->w, sh = (int)s->h;
    uint32_t* px = (uint32_t*)s->pixels;
    int stride = (int)(s->pitch / 4);
    for (int dy = 0; dy < VIEW_SRC * view_z; dy++) {
        int yy = oy + dy;
        if (yy < 0 || yy >= sh) continue;
        int sy = dy / view_z;
        for (int dx = 0; dx < VIEW_SRC * view_z; dx++) {
            int xx = ox + dx;
            if (xx < 0 || xx >= sw) continue;
            int sx = dx / view_z;
            px[yy * stride + xx] = view_src[(uint32_t)sy * VIEW_SRC + (uint32_t)sx];
        }
    }
    comp_damage_all(cid);
}

int view_set_zoom(int z) {
    if (z != 1 && z != 2) return -1;
    view_z = z;
    view_render();
    comp_composite();
    return 0;
}

/* ---------- Ayarlar ---------- */
static int set_win = -1;
static struct widget_box set_box;
static int set_box_ready = 0;

int settings_window_id(void) { return set_win; }

static void settings_status(void) {
    struct widget* st = widget_get(&set_box, 5);
    if (!st) return;
    char b[WIDGET_TXTLEN];
    strcpy(b, theme_get() == 1 ? "LIGHT " : "DARK ");
    strcat(b, theme_ui_scale() == 2 ? "2X" : "1X");
    strncpy(st->text, b, WIDGET_TXTLEN - 1);
    st->text[WIDGET_TXTLEN - 1] = 0;
}

static void settings_build(void) {
    int cid = wm_get_comp(set_win);
    struct gfx_surface* s = comp_surface(cid);
    if (!s) return;
    widget_box_init(&set_box, s);
    widget_label(&set_box, 0, "SETTINGS");
    widget_button(&set_box, 1, "DARK");
    widget_button(&set_box, 2, "LIGHT");
    widget_button(&set_box, 3, "HIDPI-1X");
    widget_button(&set_box, 4, "HIDPI-2X");
    widget_label(&set_box, 5, "DARK 1X");
    settings_status();
    widget_render(&set_box);
    comp_damage_all(cid);
    set_box_ready = 1;
}

int settings_action(int button_id) {
    if (!set_box_ready) return -1;
    struct widget* w = 0;
    switch (button_id) {
    case 1:
        if (theme_set(0) != 0) return -1;
        break;
    case 2:
        if (theme_set(1) != 0) return -1;
        break;
    case 3:
        if (theme_set_hidpi(1) != 0) return -1;
        break;
    case 4:
        if (theme_set_hidpi(2) != 0) return -1;
        break;
    default:
        return -1;
    }
    w = widget_get(&set_box, button_id);
    if (w) w->state = 1;
    theme_apply();
    settings_status();
    widget_render(&set_box);
    {
        int cid = wm_get_comp(set_win);
        if (cid >= 0) comp_damage_all(cid);
    }
    comp_composite();
    return 0;
}

/* ---------- Başlatıcı ---------- */
int apps_launch(const char* exec) {
    if (!apps_on || !exec) return -1;
    int id = -1;
    if (strcmp(exec, "edit") == 0) id = edit_win;
    else if (strcmp(exec, "view") == 0) id = view_win;
    else if (strcmp(exec, "settings") == 0) id = set_win;
    else if (strcmp(exec, "term") == 0 || strcmp(exec, "sh") == 0)
        id = term_window_id();
    else if (strcmp(exec, "files") == 0 || strcmp(exec, "ls") == 0)
        id = fm_window_id();
    else return -1;
    if (id < 0) return -1;
    wm_focus(id);
    comp_composite();
    return id;
}

int apps_init(void) {
    if (apps_on) return 0;
    edit_win = wm_create("EDITOR", 340, 220, EDIT_BG);
    if (edit_win < 0) return -1;
    view_win = wm_create("VIEWER", 300, 220, VIEW_BG);
    if (view_win < 0) return -1;
    set_win = wm_create("SETTINGS", 260, 180, 0x00181818);
    if (set_win < 0) return -1;
    wm_move(edit_win, 60, 300);
    wm_move(view_win, 430, 420);
    wm_move(set_win, 740, 548);
    view_gen();
    edit_open("/edit.txt");
    edit_render();
    view_render();
    settings_build();
    menu_add_app("Editor", "edit", "E");
    menu_add_app("Viewer", "view", "V");
    comp_composite();
    apps_on = 1;
    serial_puts("[18S] ornek uygulamalar hazır\n");
    return 0;
}

int apps_selftest(void) {
    if (!apps_on) {
        serial_puts("[18S] apps test atlandı\n");
        return -1;
    }
    int ok = 1;
    if (edit_win < 0 || view_win < 0 || set_win < 0) return -1;

    /* 1) Editör: yaz -> kaydet -> geri oku */
    if (edit_open("/edit.txt") != 0) ok = 0;
    if (ok && edit_set_line(0, "HELLO") != 0) ok = 0;
    if (ok && edit_set_line(1, "EDIT-TEST-123") != 0) ok = 0;
    if (ok && edit_set_line(99, "X") == 0) ok = 0;
    int wb = edit_save();
    if (wb <= 0) ok = 0;
    if (ok && !edit_dirty()) { /* kaydedince temizlenmeli */ }
    else if (ok) ok = 0;
    {
        char raw[64];
        int n = chfs_read_file("/edit.txt", raw, sizeof(raw) - 1);
        if (n <= 0) ok = 0;
        else {
            raw[n] = 0;
            if (strncmp(raw, "HELLO\nEDIT-TEST-123", 19) != 0) ok = 0;
        }
    }
    if (ok && edit_open("/edit.txt") != 0) ok = 0;
    if (ok && edit_nlines() != 2) ok = 0;
    {
        char l0[EDIT_COLS + 1], l1[EDIT_COLS + 1];
        if (edit_get_line(0, l0, sizeof(l0)) != 0) ok = 0;
        if (edit_get_line(1, l1, sizeof(l1)) != 0) ok = 0;
        if (ok && (strcmp(l0, "HELLO") != 0 || strcmp(l1, "EDIT-TEST-123") != 0))
            ok = 0;
        if (edit_get_line(-1, l0, sizeof(l0)) == 0) ok = 0;
    }
    /* Editör render: ilk satırda zeminden farklı piksel olmalı */
    edit_render();
    comp_composite();
    if (ok) {
        int cid = wm_get_comp(edit_win);
        struct gfx_surface* s = comp_surface(cid);
        if (s) {
            int found = 0;
            int stride = (int)(s->pitch / 4);
            for (int x = 4; x < 60 && x < (int)s->w; x++) {
                uint32_t p = ((uint32_t*)s->pixels)[12 * stride + x];
                if ((p & 0x00FFFFFFu) != (EDIT_BG & 0x00FFFFFFu)) { found = 1; break; }
            }
            if (!found) ok = 0;
        } else ok = 0;
    }

    /* 2) Görüntüleyici: zoom pikselleri */
    if (view_set_zoom(1) != 0) ok = 0;
    if (ok) {
        int cid = wm_get_comp(view_win);
        struct gfx_surface* s = comp_surface(cid);
        if (s) {
            int stride = (int)(s->pitch / 4);
            uint32_t* px = (uint32_t*)s->pixels;
            /* src(10,10): köşegen -> beyaz */
            if ((px[(24 + 10) * stride + (8 + 10)] & 0x00FFFFFFu) != 0x00FFFFFFu)
                ok = 0;
            /* src(2,0): dama + gradyan -> 0x484044 */
            if ((px[24 * stride + (8 + 2)] & 0x00FFFFFFu) != 0x00484044u) ok = 0;
        } else ok = 0;
    }
    if (view_set_zoom(2) != 0) ok = 0;
    if (ok) {
        int cid = wm_get_comp(view_win);
        struct gfx_surface* s = comp_surface(cid);
        if (s) {
            int stride = (int)(s->pitch / 4);
            uint32_t* px = (uint32_t*)s->pixels;
            uint32_t a = px[(24 + 20) * stride + (8 + 20)] & 0x00FFFFFFu;
            uint32_t b = px[(24 + 21) * stride + (8 + 21)] & 0x00FFFFFFu;
            if (a != 0x00FFFFFFu || b != 0x00FFFFFFu) ok = 0; /* 2x2 beyaz blok */
        } else ok = 0;
    }
    if (view_set_zoom(3) == 0) ok = 0;
    if (view_zoom() != 2) ok = 0;
    if (view_set_zoom(1) != 0) ok = 0;

    /* 3) Ayarlar: tema/hidpi düğmeleri + geri alma */
    if (settings_action(2) != 0) ok = 0;
    if (theme_get() != 1) ok = 0;
    if (ok) {
        struct gfx_surface* ts = comp_surface(desk_taskbar_id());
        if (ts) {
            if ((((uint32_t*)ts->pixels)[0] & 0x00FFFFFFu) != 0x00E0E0E0u) ok = 0;
        } else ok = 0;
    }
    if (settings_action(1) != 0) ok = 0;
    if (theme_get() != 0) ok = 0;
    if (settings_action(4) != 0) ok = 0;
    if (theme_ui_scale() != 2) ok = 0;
    if (settings_action(3) != 0) ok = 0;
    if (theme_ui_scale() != 1) ok = 0;
    if (settings_action(99) == 0) ok = 0;
    if (ok) {
        struct gfx_surface* ts = comp_surface(desk_taskbar_id());
        if (ts) {
            if ((((uint32_t*)ts->pixels)[0] & 0x00FFFFFFu) != 0x001A1A1Au) ok = 0;
        } else ok = 0;
    }

    /* 4) Başlatıcı eşlemesi + menü kayıtları */
    if (apps_launch("edit") != edit_win || wm_focused() != edit_win) ok = 0;
    if (apps_launch("view") != view_win || wm_focused() != view_win) ok = 0;
    if (apps_launch("settings") != set_win || wm_focused() != set_win) ok = 0;
    if (apps_launch("term") != term_window_id()) ok = 0;
    if (apps_launch("files") != fm_window_id()) ok = 0;
    if (apps_launch("bogus") != -1) ok = 0;
    {
        int n = 0;
        const struct desktop_app* l = menu_list(&n);
        int fe = 0, fv = 0;
        for (int i = 0; i < n; i++) {
            if (strcmp(l[i].name, "Editor") == 0 &&
                strcmp(l[i].exec, "edit") == 0) fe = 1;
            if (strcmp(l[i].name, "Viewer") == 0 &&
                strcmp(l[i].exec, "view") == 0) fv = 1;
        }
        if (!fe || !fv) ok = 0;
    }
    apps_launch("term"); /* odağı terminale geri ver */

    if (ok) {
        serial_puts("[18S] apps editor/viewer/settings/launcher [PASS]\n");
        vga_puts("[18S] apps editor/viewer/settings/launcher [PASS]\n");
        return 0;
    }
    serial_puts("[18S] apps [FAIL]\n");
    vga_puts("[18S] apps [FAIL]\n");
    return -1;
}
