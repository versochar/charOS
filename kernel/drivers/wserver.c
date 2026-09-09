#include <drivers/wserver.h>
#include <drivers/wm.h>
#include <drivers/compositor.h>
#include <process/pipe.h>
#include <process/task.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18F: Window server - server/client ayrığı.
 * `ws_up` (client->server) ve `ws_down` (server->client) blocking pipe.
 * wserver_task kernel thread'i tek byte komutları döngüye alır; her komutta
 * composite çağırır. Kullanıcı testi pipe fd'lerini SYS_PIPE + fork ile paylaştırır;
 * burada selftest doğrudan aynı pipe'a yazar/okur (kendi row'unda). */

static struct pipe* ws_up = 0;
static struct pipe* ws_down = 0;
static int ws_on = 0;

static int ws_getc(void) { char c; return pipe_read_block(ws_up, &c, 1) == 1 ? (uint8_t)c : -1; }
static int ws_putc(int b) { char c = (char)b; return pipe_write_block(ws_down, &c, 1) == 1 ? 0 : -1; }

static void wserver_apply(int cmd) {
    switch (cmd) {
    case WCMD_CREATE: {
        int w  = ws_getc() | (ws_getc() << 8);
        int h  = ws_getc() | (ws_getc() << 8);
        int bg = ws_getc() | (ws_getc() << 8) | (ws_getc() << 16);
        int wid = wm_create("W-CLI", w > 0 ? w : 120, h > 0 ? h : 80, (uint32_t)bg);
        if (wid < 0) { ws_putc(W_RESP_ERR); break; }
        ws_putc(W_RESP_OK); ws_putc(wid);
        break; }
    case WCMD_MOVE: { int id = ws_getc();
        int x = ws_getc() | (ws_getc() << 8);
        int y = ws_getc() | (ws_getc() << 8);
        wm_move(id, x, y); ws_putc(W_RESP_OK); break; }
    case WCMD_FOCUS: { int id = ws_getc(); wm_focus(id); ws_putc(W_RESP_OK); break; }
    case WCMD_RESIZE: { int id = ws_getc();
        int w = ws_getc() | (ws_getc() << 8);
        int h = ws_getc() | (ws_getc() << 8);
        ws_putc(wm_resize(id, w, h) == 0 ? W_RESP_OK : W_RESP_ERR); break; }
    case WCMD_MINIMIZE: { int id = ws_getc(); wm_minimize(id); ws_putc(W_RESP_OK); break; }
    case WCMD_RESTORE: { int id = ws_getc(); wm_restore(id); ws_putc(W_RESP_OK); break; }
    case WCMD_CLOSE:
    case WCMD_DESTROY: { int id = ws_getc(); wm_destroy(id); ws_putc(W_RESP_OK); break; }
    case WCMD_QUERY: {
        int id = ws_getc();
        int x = 0, y = 0, w = 0, h = 0;
        extern int wm_get_rect(int id, int* x, int* y, int* w, int* h);
        if (wm_get_rect(id, &x, &y, &w, &h) != 0) { ws_putc(W_RESP_ERR); break; }
        ws_putc(W_RESP_OK);
        ws_putc(x & 0xFF); ws_putc((x >> 8) & 0xFF);
        ws_putc(y & 0xFF); ws_putc((y >> 8) & 0xFF);
        ws_putc(w & 0xFF); ws_putc((w >> 8) & 0xFF);
        break; }
    default: ws_putc(W_RESP_ERR); break;
    }
}

static void wserver_task(void) {
    for (;;) {
        int c = ws_getc();
        if (c < 0) continue;          /* pipe eof/seal: tekrar dene (yazan gelince okur) */
        wserver_apply(c);
        comp_composite();
    }
}

static int ws_task_started = 0;

int wserver_init(void) {
    if (wm_init() != 0) return -1;
    ws_up = pipe_create();
    ws_down = pipe_create();
    if (!ws_up || !ws_down) return -1;
    ws_on = 1;
    serial_puts("[18F] wserver pipes hazır\n");
    return 0;
}

/* Scheduler çalışmaya başladıktan sonra çağrılır: ayrık wserver task'ını başlatır.
 * Bu noktadan itibaren client'lar async da iletişebilir; gönderiler ws_process_one
 * olmadan da işlenmeye başlar. */
void wserver_start_task(void) {
    if (!ws_on || ws_task_started) return;
    task_create(wserver_task, "wserver");
    ws_task_started = 1;
    serial_puts("[18F] wserver task started\n");
}

/* ---------- Global IPC pipe'ları (user -> wserver, wserver -> user) ----------
 * Bunlar wserver_task'ın okuyup/yazdığı aynı pipe'lardır; user programı
 * SYS_WSEND/SYS_WRECV ile erişir. Tek bağlantılı basit protokol.
 */
uint32_t ws_user_send_byte(int b) {
    uint8_t c = (uint8_t)b;
    if (pipe_write_block(ws_up, (char*)&c, 1) != 1) return 0xFFFFFFFF;
    return b;
}
uint32_t ws_user_recv_byte(void) {
    uint8_t c;
    if (pipe_read_block(ws_down, (char*)&c, 1) != 1) return 0xFFFFFFFF;
    return c;
}

/* Tek komut işle + composite (wserver run döngüsünün bir adımı).
 * Selftest bunu çağırarak gerçek IPC kod yolunu sürer; ayrı task'tan bağımsız,
 * ama create/move/destroy'u gerçek pipe + protokol üstünden yapar. */
void ws_process_one(void) {
    int c = ws_getc();
    if (c < 0) return;
    wserver_apply(c);
    comp_composite();
}

/* selftest: pipe üzerinden komut gönder + yanıt al (sync, kendi rowun'da) */
int wserver_selftest(void) {
    if (!ws_on) { serial_puts("[18F] wserver test skipped\n"); return -1; }
    uint8_t cmd[8];
    cmd[0] = WCMD_CREATE; cmd[1] = 140; cmd[2] = 0; cmd[3] = 90; cmd[4] = 0;
    cmd[5] = 0xFF; cmd[6] = 0x00; cmd[7] = 0x00;
    for (int i = 0; i < 8; i++) ws_user_send_byte(cmd[i]);
    ws_process_one(); /* server'ın reply'ını ws_down'a yazar */
    int r0 = ws_user_recv_byte(); int wid = ws_user_recv_byte();
    int ok = (r0 == W_RESP_OK && wid > 0);

    if (ok) {
        /* move(30,25) + query ile client->server round-trip de doğrula */
        uint8_t mcmd[6] = {WCMD_MOVE, (uint8_t)wid, 30, 0, 25, 0};
        for (int i = 0; i < 6; i++) ws_user_send_byte(mcmd[i]);
        ws_process_one();
        (void)ws_user_recv_byte();
        uint8_t qcmd[2] = {WCMD_QUERY, (uint8_t)wid};
        for (int i = 0; i < 2; i++) ws_user_send_byte(qcmd[i]);
        ws_process_one();
        int rr = ws_user_recv_byte();
        int x = ws_user_recv_byte() | (ws_user_recv_byte() << 8);
        int y = ws_user_recv_byte() | (ws_user_recv_byte() << 8);
        ok = ok && (rr == W_RESP_OK && x == 30 && y == 25);
        uint8_t dcmd[2] = {WCMD_DESTROY, (uint8_t)wid};
        for (int i = 0; i < 2; i++) ws_user_send_byte(dcmd[i]);
        ws_process_one();
        (void)ws_user_recv_byte();
    }
    if (ok) {
        serial_puts("[18F] wserver create/move/query [PASS]\n");
        vga_puts("[18F] wserver create/move/query [PASS]\n");
        return 0;
    }
    serial_puts("[18F] wserver [FAIL]\n"); vga_puts("[18F] wserver [FAIL]\n");
    return -1;
}

void wserver_run_once(void) { }
