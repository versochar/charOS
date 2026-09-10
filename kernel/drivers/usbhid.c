#include <drivers/usbhid.h>
#include <drivers/usbdesc.h>
#include <drivers/xhci.h>
#include <drivers/keyboard.h>
#include <drivers/input.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <string.h>

/* 26C: USB HID boot protokolü (klavye 1 / fare 2).
 * Numaralandırma: port reset -> slot -> adres -> tanımlayıcılar ->
 * configure EP -> SET_CONFIGURATION -> SET_PROTOCOL/IDLE -> IN TD kur.
 * Raporlar: klavye 8B boot, fare 3B+ boot. */

#define HID_MAX_DEV 2

struct hiddev {
    uint8_t used;
    uint8_t slot;
    uint8_t port;      /* 0-based port */
    uint8_t proto;     /* 1=kbd, 2=mouse */
    uint8_t iface;
    uint8_t dci;       /* kesme IN DCI */
    uint16_t ep_mps;
    uint8_t ep_interval;
    uint16_t vid, pid;
    uint8_t prev_k[8]; /* klavye: önceki rapor */
};

static struct hiddev hdev[HID_MAX_DEV];
static int hid_count = 0;
/* 64KB-hizalı rapor alanı (64KB sınır aşımı yok) */
static uint8_t report_area[256] __attribute__((aligned(65536)));

int usbhid_present(void) { return hid_count > 0; }

/* --- HID usage -> PS/2 set-1 (e0 bayraklı) --- */
struct hidmap { uint8_t ps2; uint8_t e0; };
static const struct hidmap hid2ps2[256] = {
    [0x04]= {0x1E,0},[0x05]= {0x30,0},[0x06]= {0x2E,0},[0x07]= {0x20,0},
    [0x08]= {0x12,0},[0x09]= {0x21,0},[0x0A]= {0x22,0},[0x0B]= {0x23,0},
    [0x0C]= {0x17,0},[0x0D]= {0x24,0},[0x0E]= {0x25,0},[0x0F]= {0x26,0},
    [0x10]= {0x32,0},[0x11]= {0x31,0},[0x12]= {0x18,0},[0x13]= {0x19,0},
    [0x14]= {0x10,0},[0x15]= {0x13,0},[0x16]= {0x1F,0},[0x17]= {0x14,0},
    [0x18]= {0x16,0},[0x19]= {0x2F,0},[0x1A]= {0x11,0},[0x1B]= {0x2D,0},
    [0x1C]= {0x15,0},[0x1D]= {0x2C,0},
    [0x1E]= {0x02,0},[0x1F]= {0x03,0},[0x20]= {0x04,0},[0x21]= {0x05,0},
    [0x22]= {0x06,0},[0x23]= {0x07,0},[0x24]= {0x08,0},[0x25]= {0x09,0},
    [0x26]= {0x0A,0},[0x27]= {0x0B,0},
    [0x28]= {0x1C,0},[0x29]= {0x01,0},[0x2A]= {0x0E,0},[0x2B]= {0x0F,0},
    [0x2C]= {0x39,0},[0x2D]= {0x0C,0},[0x2E]= {0x0D,0},[0x2F]= {0x1A,0},
    [0x30]= {0x1B,0},[0x31]= {0x2B,0},[0x33]= {0x27,0},[0x34]= {0x28,0},
    [0x35]= {0x29,0},[0x36]= {0x33,0},[0x37]= {0x34,0},[0x38]= {0x35,0},
    [0x39]= {0x3A,0},
    [0x3A]= {0x3B,0},[0x3B]= {0x3C,0},[0x3C]= {0x3D,0},[0x3D]= {0x3E,0},
    [0x3E]= {0x3F,0},[0x3F]= {0x40,0},[0x40]= {0x41,0},[0x41]= {0x42,0},
    [0x42]= {0x43,0},[0x43]= {0x44,0},[0x44]= {0x57,0},[0x45]= {0x58,0},
    [0x47]= {0x46,0},
    [0x49]= {0x52,1},[0x4A]= {0x47,1},[0x4B]= {0x49,1},[0x4C]= {0x53,1},
    [0x4D]= {0x4F,1},[0x4E]= {0x51,1},[0x4F]= {0x4D,1},[0x50]= {0x4B,1},
    [0x51]= {0x50,1},[0x52]= {0x48,1},
    [0xE0]= {0x1D,0},[0xE1]= {0x2A,0},[0xE2]= {0x38,0},
    [0xE4]= {0x1D,1},[0xE5]= {0x36,0},[0xE6]= {0x38,1},
};

static void hid_emit(uint8_t usage, int pressed) {
    struct hidmap m = hid2ps2[usage];
    if (!m.ps2) return;
    if (pressed) {
        if (m.e0) keyboard_scancode(0xE0);
        keyboard_scancode(m.ps2);
    } else {
        if (m.e0) keyboard_scancode(0xE0);
        keyboard_scancode(m.ps2 | 0x80);
    }
}

/* Klavye boot raporu (8B) -> basma/bırakma olayları */
static void kbd_report(struct hiddev* hd, const uint8_t* r, uint32_t len) {
    if (len < 8) return;
    /* modifier'lar (0xE0+i bit i) */
    uint8_t mods = r[0], pm = hd->prev_k[0];
    for (int i = 0; i < 8; i++) {
        int was = (pm >> i) & 1, is = (mods >> i) & 1;
        if (was == is) continue;
        hid_emit((uint8_t)(0xE0 + i), is);
    }
    /* tuşlar: bırakılanlar */
    for (int i = 0; i < 6; i++) {
        uint8_t k = hd->prev_k[2 + i];
        if (!k || k == 1) continue; /* 1 = hata rulo */
        int still = 0;
        for (int j = 0; j < 6; j++)
            if (r[2 + j] == k) { still = 1; break; }
        if (!still) hid_emit(k, 0);
    }
    /* basılanlar */
    for (int i = 0; i < 6; i++) {
        uint8_t k = r[2 + i];
        if (!k || k == 1) continue;
        int was = 0;
        for (int j = 0; j < 6; j++)
            if (hd->prev_k[2 + j] == k) { was = 1; break; }
        if (!was) hid_emit(k, 1);
    }
    for (int i = 0; i < 8; i++) hd->prev_k[i] = r[i];
}

/* Fare boot raporu (3B+) -> input hub */
static void mouse_report(struct hiddev* hd, const uint8_t* r, uint32_t len) {
    (void)hd;
    if (len < 3) return;
    int8_t dx = (int8_t)r[1], dy = (int8_t)r[2];
    input_mouse((int16_t)dx, (int16_t)dy, r[0] & 0x07u);
}

/* Kesme tamamlanma geri çağrısı */
static void hid_intr_cb(uint8_t slot, uint8_t dci, uint8_t* data,
                        uint32_t len, uint8_t cc) {
    (void)dci;
    if (cc != EVCC_OK && cc != EVCC_SHORT) return;
    for (int i = 0; i < HID_MAX_DEV; i++) {
        if (hdev[i].used && hdev[i].slot == slot) {
            if (hdev[i].proto == 1) kbd_report(&hdev[i], data, len);
            else mouse_report(&hdev[i], data, len);
            return;
        }
    }
}

static void usb_setup(uint8_t* s, uint8_t rt, uint8_t req, uint16_t val,
                      uint16_t idx, uint16_t len) {
    s[0] = rt; s[1] = req;
    s[2] = (uint8_t)(val & 0xFFu); s[3] = (uint8_t)(val >> 8);
    s[4] = (uint8_t)(idx & 0xFFu); s[5] = (uint8_t)(idx >> 8);
    s[6] = (uint8_t)(len & 0xFFu); s[7] = (uint8_t)(len >> 8);
}

/* Kontrol transfer (veri xhci_ctrl_buf üzerinden sahnelenir) */
static int hctrl(uint8_t slot, uint8_t rt, uint8_t req, uint16_t val,
                 uint16_t idx, uint8_t* data, uint32_t len, int dir_in,
                 uint32_t* got) {
    uint8_t s[8];
    uint8_t* stage = xhci_ctrl_buf();
    usb_setup(s, rt, req, val, idx, (uint16_t)len);
    if (len && data && !dir_in) {
        for (uint32_t i = 0; i < len && i < 4096u; i++) stage[i] = data[i];
    }
    uint32_t xfer = 0;
    int r = xhci_control(slot, s, len ? stage : 0, len, dir_in, &xfer);
    if (r == 0 && len && data && dir_in) {
        for (uint32_t i = 0; i < xfer && i < len; i++) data[i] = stage[i];
    }
    if (got) *got = xfer;
    return r;
}

static uint16_t rd16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/* Tek cihazı numaralandır + yapılandır. proto: 1=kbd 2=mouse, yoksa -1. */
static int enum_device(int port, int speed) {
    uint8_t slot = 0;
    if (xhci_cmd_enable_slot(&slot) != 0 || !slot) {
        serial_puts("[26C] slot yok\n");
        return -1;
    }
    serial_puts("[26C] port="); serial_puthex((uint32_t)port);
    serial_puts(" slot="); serial_puthex(slot);
    serial_puts(" speed="); serial_puthex((uint32_t)speed);
    serial_puts("\n");

    uint16_t ep0mps = (speed == XHCI_SPEED_LS) ? 8u :
                      (speed == XHCI_SPEED_SS) ? 512u : 64u;
    static uint32_t ictx[33 * 8];
    memset(ictx, 0, sizeof(ictx));
    xhci_make_slot_ctx(ictx, 0, (uint8_t)speed, 1, (uint8_t)(port + 1));
    xhci_make_ep0_ctx(slot, ictx, ep0mps);
    xhci_slot_commit(slot);
    if (xhci_cmd_address_device(slot, ictx, 0) != 0) {
        serial_puts("[26C] adresleme basarisiz\n");
        return -1;
    }

    /* cihaz tanımlayıcı (18B) */
    uint8_t devd[18];
    memset(devd, 0, sizeof(devd));
    uint32_t got = 0;
    /* 26C tanı (geçici): desenle doldur, DMA yazdı mı anla */
    {
        uint8_t* stage = xhci_ctrl_buf();
        for (int i = 0; i < 64; i++) stage[i] = 0xAA;
    }
    if (hctrl(slot, 0x80, 6, 0x0100, 0, devd, 18, 1, &got) != 0 || got < 18 ||
        devd[0] != 18 || devd[1] != 1) {
        serial_puts("[26C] cihaz tanim okunamadi\n");
        return -1;
    }
    uint16_t vid = rd16(devd + 8), pid = rd16(devd + 10);
    serial_puts("[26C] VID="); serial_puthex(vid);
    serial_puts(" PID="); serial_puthex(pid);
    serial_puts("\n");

    /* yapılandırma tanımlayıcı: önce 9B (toplam uzunluk), sonra tamamı */
    uint8_t cfg9[9];
    memset(cfg9, 0, sizeof(cfg9));
    if (hctrl(slot, 0x80, 6, 0x0200, 0, cfg9, 9, 1, &got) != 0 || got < 9) {
        serial_puts("[26C] cfg9 okunamadi\n");
        return -1;
    }
    uint16_t total = rd16(cfg9 + 2);
    if (total < 9 || total > 256) {
        serial_puts("[26C] cfg boyutu garip\n");
        return -1;
    }
    static uint8_t cfg[256];
    memset(cfg, 0, sizeof(cfg));
    if (hctrl(slot, 0x80, 6, 0x0200, 0, cfg, total, 1, &got) != 0 ||
        got < total) {
        serial_puts("[26C] cfg tam okunamadi\n");
        return -1;
    }
    uint8_t cfgval = cfg[5];

    /* arabirim + kesme IN uç: saf yürüyücü (33.4, usbdesc.c ile birebir) */
    int8_t iface = -1, proto = 0;
    uint8_t ep_addr = 0;
    uint16_t ep_mps = 0;
    uint8_t ep_interval = 0;
    {
        int p = 0;
        uint32_t mps = 0;
        if (usbdesc_find_hid_interrupt(cfg, total, &iface, &p, &ep_addr,
                                       &mps, &ep_interval) == 0) {
            proto = (int8_t)p;
            ep_mps = (uint16_t)mps;
        }
    }
    if (proto != 1 && proto != 2) {
        serial_puts("[26C] HID boot klavye/fare degil\n");
        return -1;
    }
    /* DCI: EP numarası (1) IN -> DCI 3 */
    uint8_t epnum = ep_addr & 0x0Fu;
    uint8_t dci = (uint8_t)(epnum * 2u + 1u);
    if (dci < 2 || dci > 31) {
        serial_puts("[26C] DCI garip\n");
        return -1;
    }

    /* Configure Endpoint (kesme IN): mevcut ctx'ten başla (EDK2),
     * ADD yalnızca yeni EP. Interval dönüşümü (125us birimi):
     * FS/LS = bInterval*8, HS/SS = 2^(bInterval-1). */
    memset(ictx, 0, sizeof(ictx));
    if (xhci_cfg_begin(slot, ictx, dci) != 0) {
        serial_puts("[26C] cfg begin basarisiz\n");
        return -1;
    }
    {
        uint32_t xint = ep_interval;
        if (speed == XHCI_SPEED_HS || speed == XHCI_SPEED_SS) {
            if (xint < 1u) xint = 1u;
            if (xint > 9u) xint = 9u;
            xint = 1u << (xint - 1u);
        } else {
            xint *= 8u;
            if (xint < 8u) xint = 8u;
            if (xint > 255u) xint = 255u;
        }
        if (xhci_intr_ep_setup(slot, dci, ep_mps, (uint8_t)xint,
                               ictx) != 0) {
            serial_puts("[26C] intr EP kurulumu basarisiz\n");
            return -1;
        }
    }
    ictx[1] = 0x1u | (1u << dci); /* ADD: slot biti ŞART (QEMU
     * Configure'da add[1:0]==0x1 ister; EP0 biti CLEAR), + yeni EP */
    xhci_slot_commit(slot);
    if (xhci_cmd_configure_ep(slot, ictx) != 0) {
        serial_puts("[26C] configure EP basarisiz cc=");
        serial_puthex(xhci_last_cc());
        serial_puts("\n");
        return -1;
    }

    /* SET_CONFIGURATION (Configure sonrası sıra denemesi) */
    if (hctrl(slot, 0x00, 9, cfgval, 0, 0, 0, 0, 0) != 0) {
        serial_puts("[26C] SET_CONFIGURATION basarisiz\n");
        return -1;
    }
    /* SET_PROTOCOL boot (stall ederse sorun değil) + SET_IDLE */
    hctrl(slot, 0x21, 0x0B, 0, (uint16_t)iface, 0, 0, 0, 0);
    hctrl(slot, 0x21, 0x0A, 0, (uint16_t)iface, 0, 0, 0, 0);

    /* kayda al + ilk IN TD'yi kur */
    int di = -1;
    for (int i = 0; i < HID_MAX_DEV; i++)
        if (!hdev[i].used) { di = i; break; }
    if (di < 0) {
        serial_puts("[26C] cihaz yuvasi dolu\n");
        return -1;
    }
    hdev[di].used = 1;
    hdev[di].slot = slot;
    hdev[di].port = (uint8_t)port;
    hdev[di].proto = (uint8_t)proto;
    hdev[di].iface = (uint8_t)iface;
    hdev[di].dci = dci;
    hdev[di].ep_mps = ep_mps;
    hdev[di].ep_interval = ep_interval;
    hdev[di].vid = vid;
    hdev[di].pid = pid;
    for (int i = 0; i < 8; i++) hdev[di].prev_k[i] = 0;
    hid_count++;
    {
        /* rapor tamponu: cihaz diziniyle eşleşir (poll aynı adresi kullanır) */
        uint8_t* rb = &report_area[di * 128];
        for (int i = 0; i < 64; i++) rb[i] = 0;
        if (xhci_intr_submit(slot, dci, rb, ep_mps > 64 ? 64 : ep_mps,
                             hid_intr_cb) != 0) {
            serial_puts("[26C] ilk IN kurulamadi\n");
        }
    }
    serial_puts(proto == 1 ? "[26C] klavye hazir\n" : "[26C] fare hazir\n");
    return proto;
}

int usbhid_init(void) {
    for (int i = 0; i < HID_MAX_DEV; i++) hdev[i].used = 0;
    hid_count = 0;
    if (!xhci_present()) {
        serial_puts("[26C] xhci yok, atlandi\n");
        return -1;
    }
    int n = xhci_port_count();
    int found = 0;
    for (int p = 0; p < n; p++) {
        int speed = xhci_port_reset_pub(p);
        if (speed < 0) continue; /* bağlı cihaz yok */
        if (speed != XHCI_SPEED_LS && speed != XHCI_SPEED_FS &&
            speed != XHCI_SPEED_HS && speed != XHCI_SPEED_SS)
            continue;
        int pr = enum_device(p, speed);
        if (pr == 1 || pr == 2) {
            found++;
            if (found >= HID_MAX_DEV) break;
        }
    }
    if (!found) {
        serial_puts("[26C] HID cihaz yok, atlandi\n");
        return -1;
    }
    return 0;
}

void usbhid_poll(void) {
    if (!xhci_present() || !hid_count) return;
    xhci_poll();
    /* tamamlanan TD'nin yerine yenisini kur (kesintisiz akış) */
    for (int i = 0; i < HID_MAX_DEV; i++) {
        if (!hdev[i].used) continue;
        uint8_t* rb = &report_area[i * 128];
        /* bekleyen yoksa kur (xhci tarafı reddederse sessiz geç) */
        xhci_intr_submit(hdev[i].slot, hdev[i].dci, rb,
                         hdev[i].ep_mps > 64 ? 64 : hdev[i].ep_mps,
                         hid_intr_cb);
    }
}

void usbhid_timer_poll(void) { usbhid_poll(); }

int usbhid_selftest(void) {
    if (!hid_count) return -1;
    /* boruları yokla: olaysız N tur + EP durumu */
    for (int i = 0; i < 20; i++) {
        usbhid_poll();
    }
    for (int i = 0; i < HID_MAX_DEV; i++) {
        if (!hdev[i].used) continue;
        serial_puts("[26C] hid dev stalk=");
        serial_puthex(hdev[i].slot);
        serial_puts(hdev[i].proto == 1 ? " kbd" : " mouse");
        serial_puts(" VID="); serial_puthex(hdev[i].vid);
        serial_puts(" PID="); serial_puthex(hdev[i].pid);
        serial_puts("\n");
    }
    serial_puts("[26C] usbhid [PASS]\n");
    vga_puts("[26C] usbhid [PASS]\n");
    return 0;
}
