#include <drivers/widget.h>
#include <drivers/serial.h>
#include <drivers/vga.h>
#include <memory/kheap.h>
#include <string.h>

/* 18G: Widget toolkit */

static int text_width(const char* s) {
    int n = 0;
    while (*s) { if (*s != ' ') n++; s++; }
    return n * 8;
}

static int count_visible(const char* s) {
    int n = 0;
    while (*s) { if (*s != ' ') n++; s++; }
    return n;
}

static void render_button(struct gfx_surface* s, struct widget* w) {
    uint32_t border = 0x00AAAAAA;
    if (w->state == 2) border = 0x00FFFFFF;
    else if (w->state == 1) border = 0x004444FF;

    gfx_fill_rect(s, w->x, w->y, w->w, w->h, 0x00222222);
    gfx_rect(s, w->x, w->y, w->w, w->h, border);

    int tw = text_width(w->text);
    int tx = w->x + (w->w - tw) / 2;
    int ty = w->y + (w->h - 8) / 2;
    gfx_text(s, tx, ty, w->text, 0x00FFFFFF);
}

static void render_label(struct gfx_surface* s, struct widget* w) {
    gfx_text(s, w->x, w->y + 1, w->text, 0x00CCCCCC);
}

static void render_checkbox(struct gfx_surface* s, struct widget* w) {
    int sz = 12;
    int bx = w->x;
    int by = w->y + (w->h - sz) / 2;
    gfx_fill_rect(s, bx, by, sz, sz, 0x00111111);
    gfx_rect(s, bx, by, sz, sz, 0x00888888);
    if (w->state)
        gfx_text(s, bx + 2, by + 1, "X", 0x0000FF00);
    gfx_text(s, w->x + sz + 6, w->y + 2, w->text, 0x00CCCCCC);
}

static void render_textbox(struct gfx_surface* s, struct widget* w) {
    gfx_fill_rect(s, w->x, w->y, w->w, w->h, 0x00080808);
    gfx_rect(s, w->x, w->y, w->w, w->h, 0x00888888);
    if (w->text[0])
        gfx_text(s, w->x + 4, w->y + (w->h - 8) / 2, w->text, 0x0000FF00);
    int cx = w->x + 4 + count_visible(w->text) * 8;
    int cy1 = w->y + 2;
    int cy2 = w->y + w->h - 3;
    gfx_line(s, cx, cy1, cx, cy2, 0x0000FF00);
}

/* -------- Box layout -------- */

void widget_box_init(struct widget_box* box, struct gfx_surface* surf) {
    memset(box, 0, sizeof(*box));
    box->surf = surf;
    box->pad = 4;
    box->cur_y = 4;
}

void widget_box_clear(struct widget_box* box) {
    int n = box->count;
    memset(box->widgets, 0, sizeof(box->widgets));
    box->count = 0;
    box->cur_y = 4;
    (void)n;
}

static int add_widget(struct widget_box* box, enum widget_type t,
                      int id, const char* text) {
    if (box->count >= WIDGET_MAX) return -1;
    struct widget* w = &box->widgets[box->count];
    memset(w, 0, sizeof(*w));
    w->type = t;
    w->id = id;
    w->visible = 1;
    int tw = text_width(text);
    strncpy(w->text, text, WIDGET_TXTLEN - 1);
    w->text[WIDGET_TXTLEN - 1] = '\0';
    switch (t) {
    case WT_LABEL:
        w->x = 4; w->y = box->cur_y;
        w->w = tw > 0 ? tw + 4 : 40;
        w->h = 10;
        break;
    case WT_BUTTON:
        w->x = 4; w->y = box->cur_y;
        w->w = tw + 20;
        w->h = 20;
        break;
    case WT_CHECKBOX:
        w->x = 4; w->y = box->cur_y;
        w->w = tw + 20;
        w->h = 14;
        break;
    case WT_TEXTBOX:
        w->x = 4; w->y = box->cur_y;
        w->w = 180;
        w->h = 14;
        break;
    default: break;
    }
    box->cur_y += w->h + box->pad;
    box->count++;
    return box->count - 1;
}

int widget_label(struct widget_box* box, int id, const char* text)
    { return add_widget(box, WT_LABEL, id, text); }
int widget_button(struct widget_box* box, int id, const char* text)
    { return add_widget(box, WT_BUTTON, id, text); }
int widget_checkbox(struct widget_box* box, int id, const char* text, int checked)
    { int i = add_widget(box, WT_CHECKBOX, id, text); if (i >= 0) box->widgets[i].state = !!checked; return i; }
int widget_textbox(struct widget_box* box, int id, const char* text)
    { int i = add_widget(box, WT_TEXTBOX, id, text); if (i >= 0) { strncpy(box->widgets[i].text, text, WIDGET_TXTLEN-1); box->widgets[i].text[WIDGET_TXTLEN-1] = '\0'; } return i; }

/* Render tüm widget'ları */
void widget_render(struct widget_box* box) {
    if (!box->surf) return;
    gfx_fill(box->surf, 0x00181818);
    for (int i = 0; i < box->count; i++) {
        struct widget* w = &box->widgets[i];
        if (!w->visible) continue;
        switch (w->type) {
        case WT_LABEL:    render_label(box->surf, w);    break;
        case WT_BUTTON:   render_button(box->surf, w);   break;
        case WT_CHECKBOX: render_checkbox(box->surf, w); break;
        case WT_TEXTBOX:  render_textbox(box->surf, w);  break;
        default: break;
        }
    }
}

/* Mouse tıklaması: widget alanı içinde mi kontrol et, tür göre eylem üret */
int widget_mouse(struct widget_box* box, int mx, int my, int btn) {
    if (btn != 1) return -1;
    for (int i = 0; i < box->count; i++) {
        struct widget* w = &box->widgets[i];
        if (!w->visible) continue;
        if (mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h) {
            switch (w->type) {
            case WT_BUTTON:   w->state = 1; break;
            case WT_CHECKBOX: w->state = !w->state; break;
            case WT_TEXTBOX:  w->state = 1; break; /* focused */
            default: break;
            }
            return w->id;
        }
    }
    return -1;
}

struct widget* widget_get(struct widget_box* box, int id) {
    for (int i = 0; i < box->count; i++)
        if (box->widgets[i].id == id) return &box->widgets[i];
    return 0;
}

/* -------- Selftest: offscreen surface validate -------- */
int widget_selftest(void) {
    int sw = 240, sh = 120;
    uint8_t* px = (uint8_t*)kmalloc((uint32_t)sw * (uint32_t)sh * 4);
    if (!px) { serial_puts("[18G] widget OOM\n"); return -1; }
    struct gfx_surface s = {px, (uint32_t)sw, (uint32_t)sh, (uint32_t)sw * 4, 4};
    struct widget_box box;
    widget_box_init(&box, &s);

    widget_label(&box, 1, "LABEL");
    widget_button(&box, 2, "CLICK");
    widget_checkbox(&box, 3, "CHK", 1);
    widget_textbox(&box, 4, "HELLO");

    widget_render(&box);

    int ok = 1;
    /* Label: 'L' pikselleri 0xCCCCCC olmalı (y ≈ 5) */
    {
        uint32_t p = ((uint32_t*)px)[5 * sw + 4];
        if ((p & 0x00FFFFFF) != 0x00CCCCCC) ok = 0;
    }
    /* Button arka planı 0x222222 */
    {
        struct widget* b = widget_get(&box, 2);
        int bx = b->x + 5, by = b->y + 5;
        uint32_t p = ((uint32_t*)px)[(uint32_t)by * sw + (uint32_t)bx];
        if ((p & 0x00FFFFFF) != 0x00222222) ok = 0;
    }
    /* Checkbox X: 0x00FF00 (checked) */
    {
        struct widget* cb = widget_get(&box, 3);
        int cx = cb->x + 2, cy = cb->y + 3;
        uint32_t p = ((uint32_t*)px)[(uint32_t)cy * sw + (uint32_t)cx];
        if ((p & 0x00FFFFFF) != 0x0000FF00) ok = 0;
    }
    /* Textbox cursor: 0x00FF00 dikey çizgi */
    {
        struct widget* tb = widget_get(&box, 4);
        int cx = tb->x + 4 + 5 * 8; /* "HELLO" = 5 chars */
        int cy = tb->y + 3;
        uint32_t p = ((uint32_t*)px)[(uint32_t)cy * sw + (uint32_t)cx];
        if ((p & 0x00FFFFFF) != 0x0000FF00) ok = 0;
    }
    /* Mouse dispatch: checkbox toggle */
    {
        struct widget* cb = widget_get(&box, 3);
        int before = cb->state;
        widget_mouse(&box, cb->x + 5, cb->y + 5, 1);
        int after = cb->state;
        if (before == after) ok = 0;
    }
    /* Render tekrar: toggled checkbox'ta X olmamalı */
    widget_render(&box);
    {
        struct widget* cb = widget_get(&box, 3);
        int cx = cb->x + 2, cy = cb->y + 3;
        uint32_t p = ((uint32_t*)px)[(uint32_t)cy * sw + (uint32_t)cx];
        if ((p & 0x00FFFFFF) == 0x0000FF00) ok = 0; /* X artık yok */
    }

    kfree(px);
    if (ok) {
        serial_puts("[18G] widget toolkit [PASS]\n");
        vga_puts("[18G] widget toolkit [PASS]\n");
        return 0;
    }
    serial_puts("[18G] widget [FAIL]\n");
    vga_puts("[18G] widget [FAIL]\n");
    return -1;
}
