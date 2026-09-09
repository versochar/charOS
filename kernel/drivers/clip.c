#include <drivers/clip.h>
#include <drivers/term.h>
#include <drivers/fm.h>
#include <drivers/wm.h>
#include <drivers/inr.h>
#include <fs/chfs.h>
#include <fs/vfs.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18P: Pano + uygulamalar arası sürükle-bırak.
 * Metin panosu sabit tamponda; dosya DnD yükü kaynak + tam yol taşır.
 * Hedef terminal ise yol metin olarak yapışır, hedef FM klasörü ise
 * chfs_move ile taşınır. */

static int clip_on = 0;
static char clip_buf[CLIP_MAX];
static int clip_len = 0;

static int dnd_active = 0;
static int dnd_src = -1;
static char dnd_path[FS_MAX_PATH];

int clip_init(void) {
    clip_len = 0;
    clip_buf[0] = 0;
    dnd_active = 0;
    dnd_src = -1;
    dnd_path[0] = 0;
    clip_on = 1;
    serial_puts("[18P] clipboard hazır\n");
    return 0;
}

int clip_copy(const char* s) {
    if (!clip_on || !s) return -1;
    int i = 0;
    while (s[i] && i < CLIP_MAX - 1) { clip_buf[i] = s[i]; i++; }
    clip_buf[i] = 0;
    clip_len = i;
    return i;
}

int clip_paste(char* out, int max) {
    if (!clip_on || !out || max <= 0) return -1;
    int n = clip_len;
    if (n > max - 1) n = max - 1;
    for (int i = 0; i < n; i++) out[i] = clip_buf[i];
    out[n] = 0;
    return n;
}

void clip_clear(void) {
    clip_len = 0;
    clip_buf[0] = 0;
}

int clip_length(void) { return clip_len; }
int clip_has_text(void) { return clip_on && clip_len > 0; }

int clip_dnd_set_path(const char* path, int src_win) {
    if (!clip_on || !path || !path[0]) return -1;
    int n = 0;
    while (n < FS_MAX_PATH - 1 && path[n]) { dnd_path[n] = path[n]; n++; }
    dnd_path[n] = 0;
    if (n == 0) return -1;
    dnd_src = src_win;
    dnd_active = 1;
    return 0;
}

void clip_dnd_cancel(void) { dnd_active = 0; }
int clip_dnd_active(void) { return dnd_active && clip_on; }

static const char* dnd_basename(const char* p) {
    const char* b = p;
    for (const char* q = p; *q; q++)
        if (*q == '/') b = q + 1;
    return b;
}

int clip_dnd_drop_to(int target_win) {
    if (!clip_on || !dnd_active) return -1;
    int T = term_window_id();
    int F = fm_window_id();
    if (target_win == T && term_ready()) {
        /* Dosya -> terminal: yol metin olarak yapışır (imleçte) */
        if (term_input(dnd_path) < 0) return -1;
        dnd_active = 0;
        return 0;
    }
    if (target_win == F) {
        /* Dosya -> FM klasörü: o anki dizine taşı */
        char dir[FS_MAX_PATH];
        fm_get_path(dir, FS_MAX_PATH);
        const char* base = dnd_basename(dnd_path);
        if (!base[0]) return -1;
        if ((int)(strlen(dir) + 1 + strlen(base)) >= FS_MAX_PATH) return -1;
        char dest[FS_MAX_PATH];
        dest[0] = 0;
        strcpy(dest, dir);
        int dl = (int)strlen(dest);
        if (dl > 0 && dest[dl - 1] != '/') strcat(dest, "/");
        strcat(dest, base);
        if (strcmp(dnd_path, dest) == 0) return -1; /* aynı yer */
        if (chfs_move(dnd_path, dest) != 0) return -1;
        dnd_active = 0;
        fm_refresh();
        return 0;
    }
    return -1;
}

int clip_selftest(void) {
    if (!clip_on) {
        serial_puts("[18P] clip test atlandı\n");
        return -1;
    }
    int ok = 1;
    int T = term_window_id();
    int F = fm_window_id();
    if (T < 0 || F < 0) {
        serial_puts("[18P] pencere yok\n");
        return -1;
    }

    /* 1) Temel kopyala/yapıştır */
    clip_clear();
    if (clip_has_text()) ok = 0;
    if (clip_copy("CHAROS-123") != 10) ok = 0;
    {
        char buf[CLIP_MAX];
        int n = clip_paste(buf, sizeof(buf));
        if (n != 10 || strcmp(buf, "CHAROS-123") != 0) ok = 0;
    }
    if (!clip_has_text() || clip_length() != 10) ok = 0;

    /* 2) Taşma güvenli kesilir, NUL-sonluluk korunur */
    {
        static char big[600];
        for (int i = 0; i < 599; i++) big[i] = 'A';
        big[599] = 0;
        if (clip_copy(big) != CLIP_MAX - 1) ok = 0;
        if (clip_buf[CLIP_MAX - 1] != 0) ok = 0;
        if (clip_length() != CLIP_MAX - 1) ok = 0;
    }

    /* 3) Ctrl+C / Ctrl+V kısayolu (odaklı terminal, inr üzerinden) */
    wm_focus(T);
    term_emit("\nCOPY-SRC"); /* taze satır, imleç satırı içerikli */
    inr_key_event(0x1D, 1);  /* ctrl bas */
    inr_key_event(0x2E, 1);  /* C bas */
    inr_key_event(0x2E, 0);
    inr_key_event(0x1D, 0);  /* ctrl bırak */
    {
        char got[CLIP_MAX];
        clip_paste(got, sizeof(got));
        if (strcmp(got, "COPY-SRC") != 0) ok = 0;
    }
    {
        int L0 = term_get_line_count();
        clip_copy("PASTE-LINE\n");
        inr_key_event(0x1D, 1);
        inr_key_event(0x2F, 1);  /* V bas */
        inr_key_event(0x2F, 0);
        inr_key_event(0x1D, 0);
        if (term_get_line_count() <= L0) ok = 0;
    }

    /* 4) Dosya DnD: FM -> terminal (yol yapışır, fs değişmez) */
    {
        int LT = term_get_line_count();
        if (wm_focused() != T) wm_focus(T);
        if (clip_dnd_set_path("/docs/readme.txt", F) != 0) ok = 0;
        if (!clip_dnd_active()) ok = 0;
        if (clip_dnd_drop_to(T) != 0) ok = 0;
        if (clip_dnd_active()) ok = 0;
        if (term_get_line_count() != LT) ok = 0; /* '\n' yok: aynı satır */
        char last[TERM_BUF_W];
        if (term_get_last_line(last, sizeof(last)) < 0) ok = 0;
        else if (strcmp(last, "/docs/readme.txt") != 0) ok = 0;
    }

    /* 5) Dosya DnD: FM -> FM klasörü (gerçek taşıma + geri alma) */
    if (ok && chfs_get_size("/docs/readme.txt") < 0) ok = 0;
    if (ok) {
        fm_set_path("/");
        if (clip_dnd_set_path("/docs/readme.txt", F) != 0) ok = 0;
        if (clip_dnd_drop_to(F) != 0) ok = 0;
        if (chfs_get_size("/readme.txt") < 0) ok = 0;
        if (chfs_get_size("/docs/readme.txt") >= 0) ok = 0;
        if (chfs_move("/readme.txt", "/docs/readme.txt") != 0) ok = 0;
        if (chfs_get_size("/docs/readme.txt") < 0) ok = 0;
        fm_refresh();
    }

    clip_clear();
    clip_dnd_cancel();

    if (ok) {
        serial_puts("[18P] clipboard/dnd/copy-paste [PASS]\n");
        vga_puts("[18P] clipboard/dnd/copy-paste [PASS]\n");
        return 0;
    }
    serial_puts("[18P] clipboard/dnd [FAIL]\n");
    vga_puts("[18P] clipboard/dnd [FAIL]\n");
    return -1;
}
