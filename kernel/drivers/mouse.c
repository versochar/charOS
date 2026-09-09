#include <drivers/mouse.h>
#include <drivers/input.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <core/pic.h>
#include <core/isr.h>

/* PS/2 fare (i8042 AUX port, 3 baytlık standart paket).
 * Canlı hareket QEMU/display gerektirir; init ACK dizisi headless'ta da
 * çalışır (cihaz emüle edilir). */

#define M_DATA    0x60
#define M_STATUS  0x64
#define M_CMD     0x64

#define ST_OBF  0x01  /* çıkış tamponu dolu (okunacak veri var) */
#define ST_IBF  0x02  /* giriş tamponu dolu (yazma; bekle) */
#define ST_AUX  0x20  /* veri fareden (0=klavye) */

#define M_ACK    0xFA
#define M_RESEND 0xFE

#define M_WAIT_N 200000  /* her bekleyiş üst sınırı (takılma yok) */

static inline uint8_t m_inb(uint16_t port) {
    uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static inline void m_outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline void m_delay(void) {
    /* 0x80 POST portu okuma: zararsız kısa gecikme */
    (void)m_inb(0x80);
}

static int m_present = 0;

/* Giriş tamponu boşalana kadar bekle (yazılabilir). 1 hazır, 0 timeout. */
static int m_wait_in(void) {
    for (volatile int i = 0; i < M_WAIT_N; i++) {
        if (!(m_inb(M_STATUS) & ST_IBF)) return 1;
        if ((i & 0xFFF) == 0) asm volatile("pause");
    }
    return 0;
}

/* Çıkış tamponu dolana kadar bekle (okunabilir). 1 hazır, 0 timeout. */
static int m_wait_out(void) {
    for (volatile int i = 0; i < M_WAIT_N; i++) {
        if (m_inb(M_STATUS) & ST_OBF) return 1;
        if ((i & 0xFFF) == 0) asm volatile("pause");
    }
    return 0;
}

/* Bekleyen baytları çöpe at */
static void m_flush(void) {
    for (int i = 0; i < 64; i++) {
        if (!(m_inb(M_STATUS) & ST_OBF)) break;
        (void)m_inb(M_DATA);
        m_delay();
    }
}

static int m_write_cmd(uint8_t cmd) {
    if (!m_wait_in()) return 0;
    m_outb(M_CMD, cmd);
    return 1;
}

/* Fareye bayt gönder (0xD4 öneki). 1 yazıldı. */
static int m_aux_write(uint8_t b) {
    if (!m_write_cmd(0xD4)) return 0;
    if (!m_wait_in()) return 0;
    m_outb(M_DATA, b);
    return 1;
}

/* Fare yanıt baytı oku: OBF + AUX bitli olmalı (klavye baytları atlanır).
 * 1 okundu (*out geçerli), 0 timeout. */
static int m_aux_read(uint8_t* out) {
    for (volatile int i = 0; i < M_WAIT_N; i++) {
        uint8_t st = m_inb(M_STATUS);
        if (st & ST_OBF) {
            uint8_t b = m_inb(M_DATA);
            if (st & ST_AUX) {
                if (out) *out = b;
                return 1;
            }
            /* Klavye baytı: çöpe at, fareyi beklemeye devam */
            continue;
        }
        if ((i & 0xFFF) == 0) asm volatile("pause");
    }
    return 0;
}

/* Komut gönder + ACK bekle (RESEND'de 3 deneme). 1 ACK. */
static int m_cmd_ack(uint8_t cmd) {
    for (int t = 0; t < 3; t++) {
        if (!m_aux_write(cmd)) return 0;
        uint8_t r = 0;
        if (!m_aux_read(&r)) return 0;
        if (r == M_ACK) return 1;
        if (r != M_RESEND) return 0;
    }
    return 0;
}

/* Veri baytlı komut (0xF3/0xE8): komut ACK + veri + veri ACK. */
static int m_cmd_data(uint8_t cmd, uint8_t data) {
    if (!m_cmd_ack(cmd)) return 0;
    if (!m_aux_write(data)) return 0;
    uint8_t r = 0;
    if (!m_aux_read(&r)) return 0;
    return r == M_ACK;
}

/* 3 baytlık paket çözümleyici durumu */
static uint8_t pkt[3];
static int pkt_idx = 0;

void mouse_feed_byte(uint8_t b) {
    if (pkt_idx == 0 && !(b & 0x08)) return; /* senkron: bit3 şart */
    pkt[pkt_idx++] = b;
    if (pkt_idx < 3) return;
    pkt_idx = 0;
    uint8_t b0 = pkt[0], b1 = pkt[1], b2 = pkt[2];
    if (b0 & 0xC0) return; /* taşma: paketi at */
    uint8_t btn = b0 & 0x07;
    int16_t dx = (b0 & 0x10) ? (int16_t)(b1 - 256) : (int16_t)b1;
    int16_t dy_raw = (b0 & 0x20) ? (int16_t)(b2 - 256) : (int16_t)b2;
    int16_t dy = (int16_t)-dy_raw; /* PS/2 yukarı-artar -> ekran aşağı-artar */
    input_mouse(dx, dy, btn);
}

void mouse_handler(struct registers* regs) {
    (void)regs;
    uint8_t st = m_inb(M_STATUS);
    if (!(st & ST_OBF)) return;
    if (!(st & ST_AUX)) return; /* klavye verisi: dokunma (kendi IRQ'su okur) */
    mouse_feed_byte(m_inb(M_DATA));
}

int mouse_present(void) { return m_present; }

void mouse_init(void) {
    m_present = 0;
    int step = 0;
    uint8_t got = 0;
    /* Yanıt baytlarını klavye IRQ'su çalmasın diye kesmeler kapalı init.
     * (VBox, kontrolör yanıtlarında IRQ1 üretir; QEMU üretmez.) */
    uint32_t eflags = 0;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    asm volatile("cli");
    /* Her iki cihazı durdur, tamponu temizle */
    m_write_cmd(0xAD); /* klavye kapa */
    m_write_cmd(0xA7); /* aux kapa */
    m_flush();
    /* AUX aç */
    step = 1; if (!m_write_cmd(0xA8)) goto done;
    /* Kontrol baytı: AUX IRQ aç (bit1), gerisine dokunma */
    step = 2; if (!m_write_cmd(0x20)) goto done;
    uint8_t cfg = 0;
    step = 3; if (!m_wait_out()) goto done;
    cfg = m_inb(M_DATA);
    cfg |= 0x02; /* AUX kesmesi */
    step = 4; if (!m_write_cmd(0x60)) goto done;
    step = 5; if (!m_wait_in()) goto done;
    m_outb(M_DATA, cfg);
    /* Klavyeyi geri aç (2B'de kurulmuştu) */
    m_write_cmd(0xAE);
    /* Fare reset: ACK + AA (BAT ok) + 00 (id) */
    step = 6; if (!m_aux_write(0xFF)) goto done;
    {
        uint8_t r = 0;
        step = 7; if (!m_aux_read(&r) || r != M_ACK) { got = r; goto done; }
        step = 8; if (!m_aux_read(&r) || r != 0xAA) { got = r; goto done; }
        step = 9; if (!m_aux_read(&r)) goto done; /* id (genelde 0x00) */
    }
    step = 10; if (!m_cmd_ack(0xF6)) goto done;        /* varsayılanlar */
    step = 11; if (!m_cmd_data(0xF3, 100)) goto done;  /* örnekleme 100/sn */
    step = 12; if (!m_cmd_data(0xE8, 2)) goto done;    /* çözünürlük 4 sayım/mm */
    step = 13; if (!m_cmd_ack(0xE6)) goto done;        /* ölçek 1:1 */
    step = 14; if (!m_cmd_ack(0xF4)) goto done;        /* veri raporlamayı aç */
    /* IRQ12'yi kaydet + maskeleri aç (slave için cascade IRQ2 şart) */
    isr_register_handler(44, mouse_handler);
    pic_clear_mask(12);
    pic_clear_mask(2); /* cascade: olmazsa slave IRQ'lar CPU'ya ulaşmaz */
    m_present = 1;
    serial_puts("[MOUSE] ps/2 fare hazır (irq12)\n");
    vga_puts("[MOUSE] ps/2 fare hazır\n");
done:
    m_write_cmd(0xAE); /* klavye yine de açık kalsın */
    if (eflags & 0x200) asm volatile("sti"); /* kesme durumunu geri ver */
    if (!m_present) {
        serial_puts("[MOUSE] init fail @");
        serial_puthex((uint32_t)step);
        serial_puts(" got=");
        serial_puthex(got);
        serial_puts("\n");
        serial_puts("[MOUSE] fare yok, atlanıyor\n");
    }
}

int mouse_selftest(void) {
    struct input_event ev;
    while (input_pop(&ev)) { } /* drenaj */
    int ok = 1;

    /* 0) Hub birleştirme: art arda hareketler tek olayda toplanır */
    input_push(IN_EV_MOUSE_MOVE, 0, 3, 0);
    input_push(IN_EV_MOUSE_MOVE, 0, 4, 0);
    input_push(IN_EV_MOUSE_MOVE, 1, 0, 2);
    if (!input_pop(&ev)) ok = 0;
    else {
        if (ev.type != IN_EV_MOUSE_MOVE) ok = 0;
        if (ev.x != 7 || ev.y != 2) ok = 0;
        if (ev.code != 1) ok = 0;
    }
    if (input_pending()) ok = 0;
    /* Tür değişince zincir kırılır: KEY + MOVE ayrı olay kalır */
    input_push(IN_EV_KEY_PRESS, 0x1E, 0, 0);
    input_push(IN_EV_MOUSE_MOVE, 0, 9, 9);
    if (!input_pop(&ev) || ev.type != IN_EV_KEY_PRESS) ok = 0;
    if (!input_pop(&ev)) ok = 0;
    else if (ev.type != IN_EV_MOUSE_MOVE || ev.x != 9 || ev.y != 9) ok = 0;
    if (input_pending()) ok = 0;

    /* 1) Hareket + sol tuş: {0x29, 0x05, 0xFC} */
    mouse_feed_byte(0x29);
    mouse_feed_byte(0x05);
    mouse_feed_byte(0xFC);
    if (!input_pop(&ev)) ok = 0;
    else {
        if (ev.type != IN_EV_MOUSE_MOVE) ok = 0;
        if ((ev.code & 0x01) == 0) ok = 0;
        if (ev.x != 5 || ev.y != 4) ok = 0;
    }
    if (input_pending()) ok = 0;

    /* 2) Taşmalı paket atılır: bit6 set */
    mouse_feed_byte(0xC8);
    mouse_feed_byte(0x00);
    mouse_feed_byte(0x00);
    if (input_pending()) ok = 0;

    /* 3) Senkron kaybı toparlar: çöp + geçerli paket */
    mouse_feed_byte(0x00); /* bit3 yok -> yoksay */
    mouse_feed_byte(0x08);
    mouse_feed_byte(0x03);
    mouse_feed_byte(0x03);
    if (!input_pop(&ev)) ok = 0;
    else {
        if (ev.type != IN_EV_MOUSE_MOVE) ok = 0;
        if (ev.x != 3 || ev.y != -3) ok = 0;
    }
    if (input_pending()) ok = 0;

    /* 4) Hareketsiz tuş: BTN olayı */
    mouse_feed_byte(0x08);
    mouse_feed_byte(0x00);
    mouse_feed_byte(0x00);
    if (!input_pop(&ev)) ok = 0;
    else if (ev.type != IN_EV_MOUSE_BTN) ok = 0;
    if (input_pending()) ok = 0;

    /* 5) Cihaz gerçekten algılanmış olmalı (QEMU'da her zaman var) */
    if (!mouse_present()) ok = 0;

    if (ok) {
        serial_puts("[MOUSE] ps/2 packet/routing [PASS]\n");
        vga_puts("[MOUSE] ps/2 packet/routing [PASS]\n");
        return 0;
    }
    serial_puts("[MOUSE] fare [FAIL]\n");
    vga_puts("[MOUSE] fare [FAIL]\n");
    return -1;
}
