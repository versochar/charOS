#ifndef CHAROS_DRIVERS_WIDGET_H
#define CHAROS_DRIVERS_WIDGET_H

#include <stdint.h>
#include <drivers/gfx.h>

/* 18G: Widget toolkit — button/label/textbox/checkbox, vertical box layout.
 * Her widget bir gfx_surface üstünde çalışır; mouse dispatch 포함. */

#define WIDGET_MAX    32
#define WIDGET_TXTLEN 32

enum widget_type { WT_NONE, WT_LABEL, WT_BUTTON, WT_TEXTBOX, WT_CHECKBOX };

struct widget {
    enum widget_type type;
    int x, y, w, h;
    char text[WIDGET_TXTLEN];
    int state;        /* button:pressed, checkbox:on, textbox:cursor_pos */
    int visible;
    int id;
};

struct widget_box {
    struct gfx_surface* surf;
    struct widget widgets[WIDGET_MAX];
    int count;
    int pad;          /* widget arası boşluk (px) */
    int cur_y;        /* sonraki ekleme y konumu */
};

void widget_box_init(struct widget_box* box, struct gfx_surface* surf);
void widget_box_clear(struct widget_box* box);
int  widget_label(struct widget_box* box, int id, const char* text);
int  widget_button(struct widget_box* box, int id, const char* text);
int  widget_checkbox(struct widget_box* box, int id, const char* text, int checked);
int  widget_textbox(struct widget_box* box, int id, const char* text);
void widget_render(struct widget_box* box);
int  widget_mouse(struct widget_box* box, int mx, int my, int btn);
struct widget* widget_get(struct widget_box* box, int id);

int widget_selftest(void);  /* offscreen surface validate. 0 PASS. */

#endif
