#include <drivers/wm.h>
#include <drivers/theme.h>
#include <drivers/compositor.h>
#include <drivers/fb.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18E: Window Manager */

#define WM_MAX 8
#define WM_FOCUS_BAR   0x003366CCu
#define WM_UNFOCUS_BAR 0x00666666u
#define WM_TITLE_FG    0x00FFFFFFu
#define WM_BORDER_C    0x00CCCCCCu

struct wm_window {
    int used;
    int id;            /* wm id */
    int frame;         /* 18N: çerçeve comp yüzey id (w x h, border+title) */
    int client;        /* 18N: istemci comp yüzey id (w-2 x h-TITLE_H) */
    int x, y, w, h;
    int minimized;
    uint32_t client_bg;
    char title[24];
};

static struct wm_window wm_list[WM_MAX];
static int wm_next_id = 1;
static int wm_on = 0;
static int wm_focus_id = -1;
static int wm_drag_id = -1;
static int wm_drag_dx = 0, wm_drag_dy = 0;

int wm_init(void) {
    extern int comp_init(void);
    if (comp_init() != 0) {
        serial_puts("[18E] compositor yok, wm atlandı\n");
        return -1;
    }
    if (wm_on) return 0; /* 18M: idempotent — pencereleri silme */
    memset(wm_list, 0, sizeof(wm_list));
    wm_next_id = 1;
    wm_focus_id = -1;
    wm_drag_id = -1;
    wm_on = 1;
    serial_puts("[18E] window manager hazır\n");
    return 0;
}

static struct wm_window* wm_get(int id) {
    for (int i = 0; i < WM_MAX; i++)
        if (wm_list[i].used && wm_list[i].id == id) return &wm_list[i];
    return 0;
}

int wm_get_rect(int id, int* x, int* y, int* w, int* h) {
    struct wm_window* ww = wm_get(id);
    if (!ww) return -1;
    if (x) *x = ww->x;
    if (y) *y = ww->y;
    if (w) *w = ww->w;
    if (h) *h = ww->h;
    return 0;
}

/* 18G: Dışarıya istemci yüzey erişimi (istemci comp id verilir) */
int wm_get_comp(int id) {
    struct wm_window* ww = wm_get(id);
    if (!ww) return -1;
    return ww->client;
}

/* 18N: çerçeve yüzey erişimi (dekorasyon/test için) */
int wm_get_frame(int id) {
    struct wm_window* ww = wm_get(id);
    if (!ww) return -1;
    return ww->frame;
}

/* Çerçeveyi çiz: sınır + başlık çubuğu (odak rengine göre) + başlık.
 * Yalnızca frame yüzeyine dokunur; istemci içeriği asla silinmez.
 * Renkler 18R temasından (dark varsayılan = eski sabitler). */
static void wm_draw_frame(struct wm_window* w, int focused) {
    comp_fill(w->frame, w->client_bg); /* iç boşluklar zemin rengi */
    comp_rect(w->frame, 0, 0, w->w, w->h, theme_color(TH_BORDER));
    comp_fill_rect(w->frame, 1, 1, w->w - 2, WM_TITLE_H - 1,
                   focused ? theme_color(TH_TITLE_FOCUS)
                           : theme_color(TH_TITLE_UNFOCUS));
    comp_text(w->frame, 3, 2, w->title, theme_color(TH_TITLE_FG));
}

/* 18R: tema değişiminde tüm çerçeveleri güncelle */
void wm_refresh_frames(void) {
    if (!wm_on) return;
    for (int i = 0; i < WM_MAX; i++)
        if (wm_list[i].used)
            wm_draw_frame(&wm_list[i], wm_list[i].id == wm_focus_id);
}

int wm_create(const char* title, int w, int h, uint32_t client_bg) {
    if (!wm_on || !title || w <= WM_TITLE_H + 4 || h <= WM_TITLE_H + 4) return -1;
    if (w > 1024 || h > 1024) return -1;
    if (w < 4 || h <= WM_TITLE_H) return -1;
    for (int i = 0; i < WM_MAX; i++) {
        if (wm_list[i].used) continue;
        int fid = comp_create((uint32_t)w, (uint32_t)h);
        if (fid < 0) return -1;
        int cid = comp_create((uint32_t)(w - 2), (uint32_t)(h - WM_TITLE_H));
        if (cid < 0) { comp_destroy(fid); return -1; }
        wm_list[i].used = 1;
        wm_list[i].id = wm_next_id++;
        wm_list[i].frame = fid;
        wm_list[i].client = cid;
        wm_list[i].x = wm_list[i].y = 0;
        wm_list[i].w = w; wm_list[i].h = h;
        wm_list[i].minimized = 0;
        wm_list[i].client_bg = client_bg;
        int n = 0;
        while (n < 23 && title[n]) { wm_list[i].title[n] = title[n]; n++; }
        wm_list[i].title[n] = '\0';
        comp_move(cid, 1, WM_TITLE_H);      /* istemci çerçeve içinde */
        comp_fill(cid, wm_list[i].client_bg);  /* istemci arka planı başlat */
        wm_draw_frame(&wm_list[i], 0);
        return wm_list[i].id;
    }
    return -1;
}

void wm_destroy(int id) {
    struct wm_window* w = wm_get(id);
    if (!w) return;
    comp_destroy(w->frame);
    comp_destroy(w->client);
    if (wm_focus_id == id) wm_focus_id = -1;
    if (wm_drag_id == id) wm_drag_id = -1;
    memset(w, 0, sizeof(*w));
}

void wm_focus(int id) {
    struct wm_window* w = wm_get(id);
    if (!wm_on || !w || w->minimized) return;
    if (wm_focus_id == id) return;
    struct wm_window* old = wm_get(wm_focus_id);
    if (old) wm_draw_frame(old, 0);       /* eski başlığı söndür */
    wm_focus_id = id;
    comp_raise(w->frame);                  /* çerçeve üste ... */
    comp_raise(w->client);                 /* ... istemci en üste */
    wm_draw_frame(w, 1);                   /* yeni başlığı yak */
}

int wm_focused(void) { return wm_focus_id; }

void wm_move(int id, int x, int y) {
    struct wm_window* w = wm_get(id);
    if (!w) return;
    w->x = x; w->y = y;
    comp_move(w->frame, x, y);
    comp_move(w->client, x + 1, y + WM_TITLE_H);
}

void wm_drag_start(int id, int x, int y) {
    struct wm_window* w = wm_get(id);
    if (!w || w->minimized) { wm_drag_id = -1; return; }
    wm_drag_id = id;
    wm_drag_dx = x - w->x;
    wm_drag_dy = y - w->y;
    wm_focus(id);
}

void wm_drag_to(int x, int y) {
    if (wm_drag_id < 0) return;
    struct wm_window* w = wm_get(wm_drag_id);
    if (!w) { wm_drag_id = -1; return; }
    wm_move(wm_drag_id, x - wm_drag_dx, y - wm_drag_dy);
}

/* 18M: sürüklemeyi bitir */
void wm_drag_end(void) { wm_drag_id = -1; }
int wm_dragging(void) { return wm_drag_id; }

/* Kısıt-3 (vsh windows komutu): pencere listesi + başlıklar. */
int wm_list_ids(int* out, int max) {
    if (!out || max <= 0) return 0;
    int n = 0;
    for (int i = 0; i < WM_MAX && n < max; i++)
        if (wm_list[i].used) out[n++] = wm_list[i].id;
    return n;
}

const char* wm_get_title(int id) {
    struct wm_window* w = wm_get(id);
    if (!w) return 0;
    return w->title;
}

/* 18M: comp yüzeyinden wm pencere kimliği (-1 yok) */
int wm_from_comp(int comp_id) {
    for (int i = 0; i < WM_MAX; i++)
        if (wm_list[i].used &&
            (wm_list[i].frame == comp_id || wm_list[i].client == comp_id))
            return wm_list[i].id;
    return -1;
}

/* 18M: imleç altındaki en üst pencere (minimize hariç, -1 masaüstü) */
int wm_hit_test(int x, int y) {
    if (!wm_on) return -1;
    int ids[32];
    int n = comp_zlist(ids, 32);
    for (int k = n - 1; k >= 0; k--) { /* üstten alta */
        int wid = wm_from_comp(ids[k]);
        if (wid < 0) continue;
        struct wm_window* w = wm_get(wid);
        if (!w || w->minimized) continue;
        if (x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h)
            return wid;
    }
    return -1;
}

int wm_resize(int id, int w, int h) {
    struct wm_window* wn = wm_get(id);
    if (!wn || w <= WM_TITLE_H + 4 || h <= WM_TITLE_H + 4) return -1;
    if (w > 1024 || h > 1024) return -1;
    if (comp_resize(wn->frame, (uint32_t)w, (uint32_t)h) != 0) return -1;
    if (comp_resize(wn->client, (uint32_t)(w - 2),
                    (uint32_t)(h - WM_TITLE_H)) != 0) return -1;
    wn->w = w; wn->h = h;
    comp_move(wn->client, wn->x + 1, wn->y + WM_TITLE_H);
    wm_draw_frame(wn, wn->id == wm_focus_id);
    return 0;
}

void wm_minimize(int id) {
    struct wm_window* w = wm_get(id);
    if (!w || w->minimized) return;
    w->minimized = 1;
    comp_set_visible(w->frame, 0);
    comp_set_visible(w->client, 0);
    if (wm_focus_id == id) wm_focus_id = -1;
}

void wm_restore(int id) {
    struct wm_window* w = wm_get(id);
    if (!w || !w->minimized) return;
    w->minimized = 0;
    comp_set_visible(w->frame, 1);
    comp_set_visible(w->client, 1);
    wm_focus(id);
}

int wm_selftest(void) {
    extern void comp_composite(void);
    extern void comp_set_bg(uint32_t color);
    extern uint32_t fb_getpixel_front(uint32_t x, uint32_t y);
    if (!wm_on) {
        serial_puts("[18E] selftest atlandı (wm yok)\n");
        return -1;
    }
#define W_BG    0x00222222u
#define A_CLIENT 0x00114488u
#define B_CLIENT 0x00884411u
    comp_set_bg(W_BG);
    int ok = 1;
    int A = wm_create("WIN-A", 200, 120, A_CLIENT);
    int B = wm_create("WIN-B", 120, 80, B_CLIENT);
    if (A < 0 || B < 0) {
        serial_puts("[18E] pencere OOM\n");
        return -1;
    }
    wm_move(A, 60, 50);
    wm_move(B, 150, 100);
    wm_focus(A);
    comp_composite();
    if (fb_getpixel_front(70, 70) != A_CLIENT) ok = 0;    /* A istemci */
    if (fb_getpixel_front(160, 110) != A_CLIENT) ok = 0;  /* kesişim: A üstte */
    if (fb_getpixel_front(110, 52) != WM_FOCUS_BAR) ok = 0;   /* A başlık yanık */
    /* B başlığı: A'nın kapamadığı nokta (B:150..269, A:..259 -> x 260..268) */
    if (fb_getpixel_front(265, 102) != WM_UNFOCUS_BAR) ok = 0;/* B başlık sönük */
    /* B'yi sürükle */
    wm_drag_start(B, 155, 105);
    wm_drag_to(305, 205);   /* fark (5,5) -> B (300,200)'de */
    comp_composite();
    if (wm_focused() != B) ok = 0;                        /* sürükleme odaklar */
    if (fb_getpixel_front(160, 110) != A_CLIENT) ok = 0;  /* B gitti, A görünür */
    if (fb_getpixel_front(310, 220) != B_CLIENT) ok = 0;  /* B yeni yer */
    /* A'yı küçült */
    if (wm_resize(A, 100, 60) != 0) ok = 0;
    comp_composite();
    if (fb_getpixel_front(70, 70) != A_CLIENT) ok = 0;    /* içerik korundu */
    if (fb_getpixel_front(250, 160) != W_BG) ok = 0;      /* eski alan zemin */
    /* B'yi minimize/restore et */
    wm_minimize(B);
    comp_composite();
    if (fb_getpixel_front(310, 220) != W_BG) ok = 0;
    wm_restore(B);
    comp_composite();
    if (fb_getpixel_front(310, 220) != B_CLIENT) ok = 0;
    /* Kapat */
    wm_destroy(A);
    wm_destroy(B);
    comp_composite();
    if (fb_getpixel_front(70, 70) != W_BG) ok = 0;
    if (fb_getpixel_front(310, 220) != W_BG) ok = 0;
    if (ok) {
        serial_puts("[18E] wm focus/drag/resize/minimize [PASS]\n");
        vga_puts("[18E] wm focus/drag/resize/minimize [PASS]\n");
        return 0;
    }
    serial_puts("[18E] wm [FAIL]\n");
    vga_puts("[18E] wm [FAIL]\n");
    return -1;
}

/* 18N: pencere başına ayrı frame/client yüzeyi doğrulama. 0 PASS. */
int wm_surface_selftest(void) {
    extern void comp_composite(void);
    extern void comp_set_bg(uint32_t color);
    extern uint32_t fb_getpixel_front(uint32_t x, uint32_t y);
    if (!wm_on) {
        serial_puts("[18N] selftest atlandı (wm yok)\n");
        return -1;
    }
#define N_MARK  0x00ABCDEFu
#define N_MARK2 0x00123456u
    int ok = 1;
    int W = wm_create("SURF-T", 160, 110, 0x00181818);
    if (W < 0) {
        serial_puts("[18N] pencere OOM\n");
        return -1;
    }
    int fr = wm_get_frame(W);
    int cl = wm_get_comp(W);
    if (fr < 0 || cl < 0 || fr == cl) ok = 0;
    if (ok && wm_from_comp(fr) != W) ok = 0;
    if (ok && wm_from_comp(cl) != W) ok = 0;
    /* Yüzey boyutları: frame tam, client iç alan */
    if (ok) {
        int fw = 0, fh = 0, cw = 0, ch = 0;
        if (comp_get_rect(fr, 0, 0, &fw, &fh) != 0) ok = 0;
        if (comp_get_rect(cl, 0, 0, &cw, &ch) != 0) ok = 0;
        if (fw != 160 || fh != 110) ok = 0;
        if (cw != 158 || ch != 110 - WM_TITLE_H) ok = 0;
    }
    wm_move(W, 740, 420);
    wm_focus(W);
    comp_composite();
    /* Başlık yanık, istemci zeminde */
    if (ok && fb_getpixel_front(840, 425) != WM_FOCUS_BAR) ok = 0;
    /* İstemciye işaret koy: ekranda frame offset'iyle görünmeli */
    comp_fill(cl, N_MARK);
    comp_composite();
    if (ok && fb_getpixel_front(746, 436) != N_MARK) ok = 0;
    if (ok && fb_getpixel_front(840, 425) != WM_FOCUS_BAR) ok = 0;
    /* İstemciyi yeniden boya: çerçeve etkilenmemeli (18N çekirdek özellik) */
    comp_fill(cl, N_MARK2);
    comp_composite();
    if (ok && fb_getpixel_front(746, 436) != N_MARK2) ok = 0;
    if (ok && fb_getpixel_front(840, 425) != WM_FOCUS_BAR) ok = 0;
    /* Başka pencereye odaklan: başlık söner, istemci korunur */
    int W2 = wm_create("SURF-U", 120, 80, 0x00101010);
    if (W2 < 0) ok = 0;
    else {
        wm_move(W2, 100, 500);
        wm_focus(W2);
        comp_composite();
        if (fb_getpixel_front(840, 425) != WM_UNFOCUS_BAR) ok = 0;
        if (fb_getpixel_front(746, 436) != N_MARK2) ok = 0;
        wm_destroy(W2);
    }
    /* Taşı: frame+client birlikte gider */
    wm_focus(W);
    wm_move(W, 760, 440);
    comp_composite();
    if (ok && fb_getpixel_front(766, 456) != N_MARK2) ok = 0;
    if (ok && fb_getpixel_front(860, 445) != WM_FOCUS_BAR) ok = 0;
    wm_destroy(W);
    comp_composite();
    if (ok) {
        serial_puts("[18N] per-window frame/client surfaces [PASS]\n");
        vga_puts("[18N] per-window frame/client surfaces [PASS]\n");
        return 0;
    }
    serial_puts("[18N] per-window surfaces [FAIL]\n");
    vga_puts("[18N] per-window surfaces [FAIL]\n");
    return -1;
}
