#include <drivers/fm.h>
#include <drivers/wm.h>
#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <fs/chfs.h>
#include <fs/vfs.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>
#include <memory/kheap.h>

/* 18K: GUI Dosya Yöneticisi — liste, sürükle-bırak (chFS move). */

#define FM_ROW_H    18
#define FM_VIEW_W   260
#define FM_HEAD_H   12   /* yol çubuğu */
#define FM_ROWS     12   /* görünür satır sayısı */
#define FM_VIEW_H   (FM_HEAD_H + FM_ROWS * FM_ROW_H)

struct fm_row {
    char name[FS_LIST_NAME_LEN];
    int is_dir;
};

static int fm_window = -1;
static struct fm_row rows[FS_LIST_MAX];
static int row_count = 0;
static char fm_path[FS_MAX_PATH] = "/";
static int fm_drag_from = -1;   /* sürüklenen satır indeksi */
static int fm_on = 0;

static void fm_render(void) {
    if (fm_window < 0) return;
    int cid = wm_get_comp(fm_window);
    if (cid < 0) return;
    struct gfx_surface* s = comp_surface(cid);
    if (!s) return;

    gfx_fill(s, 0x00101010);

    /* Yol çubuğu */
    gfx_fill_rect(s, 0, 0, FM_VIEW_W, FM_HEAD_H, 0x00282828);
    gfx_text(s, 4, 2, fm_path, 0x00AAAAFF);

    /* Dosya listesi */
    for (int i = 0; i < row_count && i < FM_ROWS; i++) {
        struct fm_row* r = &rows[i];
        int y = FM_HEAD_H + i * FM_ROW_H;
        if (i == fm_drag_from)
            gfx_fill_rect(s, 0, y, FM_VIEW_W, FM_ROW_H, 0x00303030);
        /* İkon: klasör=mavi, dosya=yeşil kutu */
        if (r->is_dir) {
            gfx_fill_rect(s, 4, y + 3, 12, 10, 0x004444AA);
            gfx_rect(s, 4, y + 3, 12, 10, 0x00666666);
        } else {
            gfx_fill_rect(s, 4, y + 3, 10, 10, 0x0000AA44);
            gfx_rect(s, 4, y + 3, 10, 10, 0x00558855);
        }
        gfx_text(s, 24, y + 3, r->name, r->is_dir ? 0x00CCCCCC : 0x00BBBBBB);
    }
    comp_damage_all(cid);
}

void fm_refresh(void) {
    /* 19A: FS_LIST_MAX=64 ile yığın taşmasını önlemek için statik (8KB kstack) */
    static struct fs_list_entry ents[FS_LIST_MAX];
    row_count = chfs_list_dir(fm_path, ents, FS_LIST_MAX);
    for (int i = 0; i < row_count && i < FS_LIST_MAX; i++) {
        struct fm_row* r = &rows[i];
        r->is_dir = ents[i].is_dir;
        strncpy(r->name, ents[i].name, FS_LIST_NAME_LEN - 1);
        r->name[FS_LIST_NAME_LEN - 1] = 0;
    }
    fm_render();
    comp_composite();
}

void fm_set_path(const char* path) {
    if (path) {
        strncpy(fm_path, path, FS_MAX_PATH - 1);
        fm_path[FS_MAX_PATH - 1] = 0;
    }
    fm_drag_from = -1;
    fm_refresh();
}

int fm_init(void) {
    if (!wm_init()) {
        fm_window = wm_create("FILES", FM_VIEW_W + 4, FM_VIEW_H + 4 + WM_TITLE_H,
                                0x00101010);
        if (fm_window < 0) return -1;
        fm_on = 1;
        fm_refresh();
        serial_puts("[18K] file manager started\n");
        vga_puts("[18K] file manager started\n");
        return 0;
    }
    return -1;
}

int fm_window_id(void) { return fm_window; }

/* 18P: bırakma hedefi için o anki dizin */
void fm_get_path(char* out, int max) {
    if (!out || max <= 0) return;
    out[0] = 0;
    int n = 0;
    while (n < max - 1 && fm_path[n]) { out[n] = fm_path[n]; n++; }
    out[n] = 0;
}

/* Sürükle-bırak: path dizini içine "file" taşı */
static int fm_dnd_drop(int target_row, const char* path) {
    if (target_row < 0 || target_row >= row_count) return -1;
    char dest[FS_MAX_PATH];
    dest[0] = 0;
    /* hedef klasör yolu: /klasor veya /yol/klasor */
    if (strcmp(fm_path, "/") == 0) {
        strcpy(dest, "/");
        strcat(dest, rows[target_row].name);
    } else {
        strcpy(dest, fm_path);
        strcat(dest, "/");
        strcat(dest, rows[target_row].name);
    }
    /* kaynak tam yol: fm_path + / + file */
    char src[FS_MAX_PATH];
    if (strcmp(fm_path, "/") == 0) {
        src[0] = 0;
        strcat(src, "/");
        strcat(src, path);
    } else {
        strcpy(src, fm_path);
        strcat(src, "/");
        strcat(src, path);
    }
    char newpath[FS_MAX_PATH];
    strcpy(newpath, dest);
    strcat(newpath, "/");
    strcat(newpath, path);
    return chfs_move(src, newpath);
}

/* Sürükle başlat (satır indeksi) / bırak (forum uyg.) */
int fm_drag(int row) {
    if (row < 0 || row >= row_count) return -1;
    fm_drag_from = row;
    fm_render();
    return 0;
}

int fm_drop(int row) {
    if (fm_drag_from < 0) return -1;
    if (fm_drag_from == row) { fm_drag_from = -1; fm_render(); return 0; }
    if (!rows[row].is_dir) { fm_drag_from = -1; fm_render(); return -1; }
    int r = fm_dnd_drop(row, rows[fm_drag_from].name);
    fm_drag_from = -1;
    fm_refresh();
    return r;
}

int fm_selftest(void) {
    if (!fm_on) {
        serial_puts("[18K] fm test atlandı\n");
        return -1;
    }
    int ok = 1;

    /* Test dosyaları/klasörleri kur */
    fs_create("/readme.txt");
    fs_create("/notes.txt");
    fs_mkdir("/docs");

    /* Listeleme: / içinde en az 3 giriş */
    static struct fs_list_entry ents[FS_LIST_MAX];
    int n = chfs_list_dir("/", ents, FS_LIST_MAX);
    if (n < 3) ok = 0;

    /* docs dizinini bul */
    int docs_idx = -1;
    for (int i = 0; i < n; i++)
        if (strcmp(ents[i].name, "docs") == 0 && ents[i].is_dir) docs_idx = i;
    if (docs_idx < 0) ok = 0;

    /* Sürükle-bırak: readme.txt -> docs (chfs_move /readme.txt -> /docs/readme.txt) */
    fm_set_path("/");
    fm_drag(0);
    int dnd = fm_dnd_drop(docs_idx, "readme.txt");
    if (dnd != 0) ok = 0;

    /* Taşınmış: /readme.txt artık yok, /docs/readme.txt var */
    if (ok && chfs_get_size("/readme.txt") >= 0) ok = 0;
    if (ok && chfs_get_size("/docs/readme.txt") < 0) ok = 0;

    /* ikinci dosyayı da taşı */
    int mv = chfs_move("/notes.txt", "/docs/notes.txt");
    if (mv != 0) ok = 0;
    if (ok && chfs_get_size("/notes.txt") >= 0) ok = 0;
    if (ok && chfs_get_size("/docs/notes.txt") < 0) ok = 0;

    /* Render: tercihen yeşil dosya ikonu pikseli */
    if (ok) {
        int cid = wm_get_comp(fm_window);
        struct gfx_surface* s = comp_surface(cid);
        if (s) {
            uint32_t p = ((uint32_t*)s->pixels)[(FM_HEAD_H + 5) * (s->pitch / 4) + 5];
            /* dosya ikonu (0x00AA44) ya da klasör (0x4444AA) fark etmez, sadece çizilmiş olmalı */
            if ((p & 0x00FFFFFF) == 0x00101010) ok = 0;
        } else ok = 0;
    }

    if (ok) {
        serial_puts("[18K] file manager/list/dnd/move [PASS]\n");
        vga_puts("[18K] file manager/list/dnd/move [PASS]\n");
        return 0;
    }
    serial_puts("[18K] file manager [FAIL]\n");
    vga_puts("[18K] file manager [FAIL]\n");
    return -1;
}
