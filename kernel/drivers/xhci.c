#include <drivers/xhci.h>
#include <drivers/pci.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <memory/paging.h>
#include <core/spinlock.h>
#include <string.h>

/* 26B: xHCI USB3 host controller. QEMU qemu-xhci + Intel Tiger Lake hedefi.
 * Polling modeli: komut/olay halkaları timer/ana döngüden taranır (MSI yok).
 * Bit konumları Linux xhci.h (kanonik) ile doğrulandı. */

/* --- MMIO --- */
static volatile uint8_t* x_base = 0;
static uint32_t x_oplen = 0, x_dboff = 0, x_rtsoff = 0;
static uint8_t x_nports = 0, x_nslots = 0;

static inline uint32_t r32(uint32_t off) {
    return *(volatile uint32_t*)(x_base + off);
}
static inline void w32(uint32_t off, uint32_t v) {
    *(volatile uint32_t*)(x_base + off) = v;
}
static inline uint64_t r64(uint32_t off) {
    return *(volatile uint64_t*)(x_base + off);
}
static inline void w64(uint32_t off, uint64_t v) {
    *(volatile uint64_t*)(x_base + off) = v;
}
static inline uint32_t op32(uint32_t off) { return r32(x_oplen + off); }
static inline void opw32(uint32_t off, uint32_t v) { w32(x_oplen + off, v); }
static inline uint64_t op64(uint32_t off) { return r64(x_oplen + off); }
static inline void opw64(uint32_t off, uint64_t v) { w64(x_oplen + off, v); }

/* --- DMA alanları (identity, düşük bellek; veri 64KB sınırına takılmaz) --- */
#define XHCI_NCMD 16
#define XHCI_NEVT 16
#define XHCI_NDEV 2

static struct xhci_trb cmd_ring[XHCI_NCMD + 1] __attribute__((aligned(64)));
static struct xhci_trb evt_ring[XHCI_NEVT + 1] __attribute__((aligned(64)));
static uint32_t erst_arr[4] __attribute__((aligned(64))); /* addr_lo,hi,size,rsvd */
static uint64_t dcbaa[256] __attribute__((aligned(64)));
static uint32_t in_ctx[33 * 8] __attribute__((aligned(64)));
static uint32_t dev_ctx[XHCI_NDEV][32 * 8] __attribute__((aligned(64)));
/* 26C-HİZA DÜZELTMESİ: her halka 64B hizalı olmalı (spec). 17 TRB=272B
 * (64'ün katı değil!) ikinci halkayı kaydırıyordu -> Configure cc=5.
 * 20 TRB=320B stride ile hepsi hizalı; ilk 17'si kullanılır. */
static struct xhci_trb ep0_ring[XHCI_NDEV][XHCI_NCMD + 4] __attribute__((aligned(64)));
static struct xhci_trb intr_ring[XHCI_NDEV][XHCI_NCMD + 4] __attribute__((aligned(64)));
/* 64KB-hizalı veri bölgesi (kontrol + HID raporları; 64KB sınır aşımı yok) */
static uint8_t xfer_data[4096] __attribute__((aligned(65536)));
/* Scratchpad (HCSPARAMS2 isterse; 32 sayfaya kadar) */
static uint64_t scratch_arr[32] __attribute__((aligned(64)));
static uint8_t scratch_pages[32][4096] __attribute__((aligned(4096)));

static int x_present = 0;
static spinlock_t x_lock = SPINLOCK_INIT;

/* halka durumları */
static uint32_t cmd_idx = 0, cmd_cycle = 1;
static uint32_t evt_idx = 0, evt_cycle = 1;

/* bekleyen komut (senkron model: tek seferde bir) */
static uint32_t pend_cmd_lo = 0, pend_cc = 0, pend_slot = 0;
static int pend_cmd_active = 0;
static uint32_t last_cc = 0;

uint8_t xhci_last_cc(void) { return (uint8_t)last_cc; }

/* bekleyen kontrol transferi (senkron model: tek seferde bir) */
static int ctrl_wait_active = 0;
static uint8_t ctrl_wait_slot = 0;
static uint32_t ctrl_wait_len = 0, ctrl_wait_cc = 0xFFu, ctrl_wait_got = 0;

/* cihaz başına EP0 + kesme durumu */
struct xhci_devst {
    uint8_t slot;
    uint8_t dci_intr;      /* kesme IN DCI (0 = yok) */
    uint16_t ep0_mps;
    uint32_t ep0_idx, ep0_cycle;
    uint32_t intr_idx, intr_cycle;
    int intr_pending;
    uint8_t* intr_buf;
    uint32_t intr_len;
    xhci_intr_cb intr_cb;
    uint32_t intr_trb_lo;
};
static struct xhci_devst xdev[XHCI_NDEV];

static uint32_t phys_of(const void* p) { return (uint32_t)(uintptr_t)p; }

static void trb_write(struct xhci_trb* r, uint64_t param, uint32_t status,
                      uint32_t control) {
    r->param = param;
    r->status = status;
    r->control = control;
}

/* Olay halkası tara: tamamlanmaları işle, ERDP güncelle */
static void evt_drain(void) {
    int work = 0;
    for (;;) {
        struct xhci_trb* e = &evt_ring[evt_idx];
        uint32_t ctrl = e->control;
        if (((ctrl & TRB_C) ? 1u : 0u) != evt_cycle) break;
        uint32_t type = (ctrl >> 10) & 0x3Fu;
        uint64_t ptr = e->param;
        uint32_t st = e->status, fl = e->control; /* ad uyumu: status=dword2 */
        /* not: control dword'u tip+cycle içerir; slot/cc flag dword'unda */
        uint32_t flags = e->control;
        (void)st; (void)flags;
        if (type == XHCI_TRB_EV_CMD) {
            /* command completion: ptr + cc(status>>24) + slot(flags>>24) */
            if ((uint32_t)ptr == pend_cmd_lo && pend_cmd_active) {
                pend_cc = (st >> 24) & 0xFFu;
                pend_slot = (fl >> 24) & 0xFFu;
                pend_cmd_active = 0;
            }
        } else if (type == XHCI_TRB_EV_TX) {
            /* transfer: ptr + residual/cc + slot/ep */
            uint32_t tcc = (st >> 24) & 0xFFu;
            uint32_t resid = st & 0xFFFFFFu;
            uint32_t eslot = (fl >> 24) & 0xFFu;
            uint32_t epid = (fl >> 16) & 0x1Fu;
            int consumed_by_ctrl = 0;
            if (ctrl_wait_active && eslot == ctrl_wait_slot && epid == 1) {
                ctrl_wait_cc = tcc;
                ctrl_wait_got = (ctrl_wait_len > resid) ?
                    (ctrl_wait_len - resid) : 0;
                ctrl_wait_active = 0;
                consumed_by_ctrl = 1;
            }
            if (!consumed_by_ctrl) {
                for (int d = 0; d < XHCI_NDEV; d++) {
                    struct xhci_devst* ds = &xdev[d];
                    if (!ds->slot || ds->slot != eslot) continue;
                    if (ds->intr_pending && epid == ds->dci_intr &&
                        (uint32_t)ptr == ds->intr_trb_lo) {
                        uint32_t got = ds->intr_len > resid ?
                            ds->intr_len - resid : 0;
                        xhci_intr_cb cb = ds->intr_cb;
                        uint8_t* buf = ds->intr_buf;
                        uint8_t sl = ds->slot, dc = ds->dci_intr;
                        ds->intr_pending = 0;
                        if (cb) cb(sl, dc, buf, got, (uint8_t)tcc);
                    }
                }
            }
        }
        /* port-status olayları atlanır (port register'dan okunur) */
        evt_idx++;
        if (evt_idx >= XHCI_NEVT) {
            evt_idx = 0;
            evt_cycle ^= 1u;
        }
        work = 1;
    }
    if (work) {
        /* ERDP güncelle (EHB=0, DESI=0) + IP temizle */
        w64(x_rtsoff + 0x20 + 0x18, (uint64_t)phys_of(&evt_ring[evt_idx]));
        w32(x_rtsoff + 0x20, IMAN_IP);
    }
}

void xhci_poll(void) {
    if (!x_present) return;
    uint32_t f;
    spin_lock_irqsave(&x_lock, &f);
    evt_drain();
    spin_unlock_irqrestore(&x_lock, f);
}

/* Komut kuyrukla + kapı çal + tamamlanmayı bekle.
 * 26B-DEADLOCK DERSİ: bekleme boyunca kilit TUTULMAZ (tick PIT IRQ'dan
 * gelir; irqsave ile beklemek zamanı dondurur). Kuyruk+kapı kilitli,
 * bekleme kilitsiz (xhci_poll içeride kilitler), sonuç kilitli okunur. */
static int cmd_issue(uint64_t param, uint32_t status, uint32_t control,
                     uint8_t* out_slot, uint32_t timeout) {
    struct xhci_trb* t;
    uint32_t tlo;
    {
        uint32_t f;
        spin_lock_irqsave(&x_lock, &f);
        t = &cmd_ring[cmd_idx];
        tlo = phys_of(t);
        trb_write(t, param, status, control | cmd_cycle);
        cmd_idx++;
        if (cmd_idx >= XHCI_NCMD) {
            cmd_idx = 0;
            cmd_cycle ^= 1u;
        }
        pend_cmd_lo = tlo;
        pend_cc = 0xFFu;
        pend_slot = 0;
        pend_cmd_active = 1;
        w32(x_dboff, 0); /* kapı: host controller */
        spin_unlock_irqrestore(&x_lock, f);
    }
    uint32_t t0 = timer_get_ticks();
    int cc = 0xFF, sl = 0, done = 0;
    for (;;) {
        xhci_poll();
        {
            uint32_t f;
            spin_lock_irqsave(&x_lock, &f);
            if (!pend_cmd_active) {
                cc = (int)pend_cc;
                sl = (int)pend_slot;
                done = 1;
            }
            spin_unlock_irqrestore(&x_lock, f);
        }
        if (done) break;
        if ((uint32_t)(timer_get_ticks() - t0) > timeout) {
            uint32_t f;
            spin_lock_irqsave(&x_lock, &f);
            pend_cmd_active = 0;
            spin_unlock_irqrestore(&x_lock, f);
            last_cc = 0xFFu;
            return -2; /* zaman aşımı */
        }
    }
    {
        uint32_t f;
        spin_lock_irqsave(&x_lock, &f);
        last_cc = (uint32_t)cc;
        spin_unlock_irqrestore(&x_lock, f);
    }
    if (out_slot) *out_slot = (uint8_t)sl;
    return (cc == EVCC_OK) ? 0 : -3;
}

int xhci_cmd_enable_slot(uint8_t* slot_id) {
    if (!x_present) return -1;
    uint8_t sl = 0;
    int r = cmd_issue(0, 0, (XHCI_TRB_EN_SLOT << 10), &sl, 100);
    if (r == 0 && slot_id) *slot_id = sl;
    return r;
}

int xhci_cmd_address_device(uint8_t slot, uint32_t* ictx, int bsr) {
    if (!x_present || !ictx) return -1;
    uint32_t ctrl = (XHCI_TRB_ADDR_DEV << 10) | ((uint32_t)slot << 24);
    if (bsr) ctrl |= TRB_BSR;
    return cmd_issue((uint64_t)phys_of(ictx), 0, ctrl, 0, 200);
}

int xhci_cmd_configure_ep(uint8_t slot, uint32_t* ictx) {
    if (!x_present || !ictx) return -1;
    uint32_t ctrl = (XHCI_TRB_CFG_EP << 10) | ((uint32_t)slot << 24);
    return cmd_issue((uint64_t)phys_of(ictx), 0, ctrl, 0, 200);
}

/* Slot context doldur (input ctx dw 8..15) */
void xhci_make_slot_ctx(uint32_t* ictx, uint32_t route, uint8_t speed,
                        uint8_t entries, uint8_t rhport) {
    ictx[0] = 0; ictx[1] = SLOT_CTX_ADD;
    ictx[8] = (route & 0xFFFFFu) | ((uint32_t)speed << 20) |
              ((uint32_t)entries << 27);
    ictx[9] = ((uint32_t)rhport << 16);
    ictx[10] = 0; ictx[11] = 0;
}

/* EP0 context doldur (dci 1 -> dw 16..19) */
void xhci_make_ep0_ctx(uint8_t slot, uint32_t* ictx, uint16_t mps) {
    int d = slot - 1;
    if (d < 0 || d >= XHCI_NDEV) return;
    struct xhci_devst* ds = &xdev[d];
    ds->slot = slot;
    ds->ep0_mps = mps;
    ds->ep0_idx = 0;
    ds->ep0_cycle = 1;
    ds->intr_pending = 0;
    ds->dci_intr = 0;
    /* transfer halkası boş, link TRB kurulu değilse kur */
    for (int i = 0; i < XHCI_NCMD; i++) {
        ep0_ring[d][i].param = 0;
        ep0_ring[d][i].status = 0;
        ep0_ring[d][i].control = 0;
    }
    trb_write(&ep0_ring[d][XHCI_NCMD], (uint64_t)phys_of(ep0_ring[d]),
              0, (XHCI_TRB_LINK << 10) | TRB_TC | 1u);
    ictx[1] |= EP0_CTX_ADD;
    ictx[16] = 0; /* interval/mult/MaxPStreams = 0 */
    ictx[17] = (3u << 1) | (EP_TYPE_CTRL << 3) | ((uint32_t)mps << 16);
    /* 26B-DCS DÜZELTMESİ: DCS, 64-bit deq'nun 0. bitidir (düşük dword!).
     * Yanlışlıkla yüksek dword'a yazılınca halka boş görünüyordu. */
    ictx[18] = phys_of(ep0_ring[d]) | 1u;
    ictx[19] = 0;
}

/* Kesme IN EP context (dci) + transfer halkası bağla */
int xhci_intr_ep_setup(uint8_t slot, uint8_t dci, uint16_t mps,
                       uint8_t interval, uint32_t* ictx) {
    int d = slot - 1;
    if (d < 0 || d >= XHCI_NDEV || !ictx || dci < 2 || dci > 31) return -1;
    struct xhci_devst* ds = &xdev[d];
    ds->dci_intr = dci;
    ds->intr_idx = 0;
    ds->intr_cycle = 1;
    ds->intr_pending = 0;
    for (int i = 0; i < XHCI_NCMD; i++) {
        intr_ring[d][i].param = 0;
        intr_ring[d][i].status = 0;
        intr_ring[d][i].control = 0;
    }
    trb_write(&intr_ring[d][XHCI_NCMD], (uint64_t)phys_of(intr_ring[d]),
              0, (XHCI_TRB_LINK << 10) | TRB_TC | 1u);
    /* 26C-DCI DÜZELTMESİ: input ctx'te EP(DCI) dw = DCI*8+8
     * (slot dw8, EP0 dw16, EP1-OUT dw24, EP1-IN dw32). Eskiden 8 eksikti:
     * EP1-IN dw24'e (OUT yerine) yazılıyordu, gerçek EP boş kalıyordu. */
    uint32_t e = (uint32_t)dci * 8u + 8u;
    ictx[1] |= (1u << dci); /* add EP */
    /* slot entries güncelle */
    {
        uint32_t entries = (ictx[8] >> 27) & 0x1Fu;
        if (dci > entries) {
            ictx[8] = (ictx[8] & ~(0x1Fu << 27)) | ((uint32_t)dci << 27);
        }
    }
    ictx[e + 0] = ((uint32_t)interval << 16);
    ictx[e + 1] = (3u << 1) | (EP_TYPE_INT_IN << 3) | ((uint32_t)mps << 16);
    ictx[e + 2] = phys_of(intr_ring[d]) | 1u; /* DCS bit0 */
    ictx[e + 3] = 0;
    /* tx_info: avg TRB uzunluğu + Max ESIT payload (kesme: mps) */
    ictx[e + 4] = ((uint32_t)mps) | ((uint32_t)mps << 16);
    return 0;
}

/* EP0 kontrol transferi (senkron). xfer_len: gerçekleşen bayt. */
int xhci_control(uint8_t slot, const uint8_t setup[8],
                 uint8_t* data, uint32_t len, int dir_in,
                 uint32_t* xfer_len) {
    int d = slot - 1;
    if (!x_present || d < 0 || d >= XHCI_NDEV || !xdev[d].slot) return -1;
    if (xfer_len) *xfer_len = 0;
    /* TD kur + kapı çal (kilitli, kısa) */
    {
        uint32_t f;
        spin_lock_irqsave(&x_lock, &f);
        struct xhci_devst* ds = &xdev[d];
        /* setup paketi */
        uint64_t spar = 0;
        for (int i = 7; i >= 0; i--) spar = (spar << 8) | setup[i];
        uint32_t trt = (len == 0) ? 0 : (dir_in ? 3u : 2u);
        /* setup: OVMF kanıtı uzunluk=8 + IDT=1 (setup baytı TRB'de!) */
        {
            struct xhci_trb* t = &ep0_ring[d][ds->ep0_idx];
            trb_write(t, spar, 8u,
                      (XHCI_TRB_SETUP << 10) | (trt << 16) | TRB_IDT |
                      ds->ep0_cycle);
            ds->ep0_idx++;
            if (ds->ep0_idx >= XHCI_NCMD) {
                ds->ep0_idx = 0; ds->ep0_cycle ^= 1u;
            }
        }
        /* data: OVMF kanıtı yalnız length (TDSize=0) */
        if (len && data) {
            struct xhci_trb* t = &ep0_ring[d][ds->ep0_idx];
            uint32_t llen = (len > 0x10000u) ? 0x10000u : len;
            /* OVMF kanıtı: yalnız length + IOC + ISP */
            trb_write(t, (uint64_t)phys_of(data), llen,
                      (XHCI_TRB_DATA << 10) | (dir_in ? TRB_DIR_IN : 0u) |
                      (1u << 2) | TRB_IOC | ds->ep0_cycle);
            ds->ep0_idx++;
            if (ds->ep0_idx >= XHCI_NCMD) {
                ds->ep0_idx = 0; ds->ep0_cycle ^= 1u;
            }
        }
        /* status (yön tersi; verisizse IN) */
        {
            int st_in = (len == 0) ? 1 : (dir_in ? 0 : 1);
            struct xhci_trb* t = &ep0_ring[d][ds->ep0_idx];
            trb_write(t, 0, 0,
                      (XHCI_TRB_STATUS << 10) | (st_in ? TRB_DIR_IN : 0u) |
                      TRB_IOC | ds->ep0_cycle);
            ds->ep0_idx++;
            if (ds->ep0_idx >= XHCI_NCMD) {
                ds->ep0_idx = 0; ds->ep0_cycle ^= 1u;
            }
        }
        ctrl_wait_active = 1;
        ctrl_wait_slot = slot;
        ctrl_wait_len = len;
        ctrl_wait_cc = 0xFFu;
        ctrl_wait_got = 0;
        w32(x_dboff + (uint32_t)slot * 4u, 1u); /* kapı: EP0 (DCI 1) */
        spin_unlock_irqrestore(&x_lock, f);
    }
    /* bekle (kilitsiz; xhci_poll içeride kilitler) */
    {
        uint32_t t0 = timer_get_ticks();
        int done = 0;
        uint32_t cc = 0xFFu, got = 0;
        for (;;) {
            xhci_poll();
            {
                uint32_t f;
                spin_lock_irqsave(&x_lock, &f);
                if (!ctrl_wait_active) {
                    cc = ctrl_wait_cc;
                    got = ctrl_wait_got;
                    done = 1;
                }
                spin_unlock_irqrestore(&x_lock, f);
            }
            if (done) break;
            if (((uint32_t)(timer_get_ticks() - t0)) >= 100u) {
                uint32_t f;
                spin_lock_irqsave(&x_lock, &f);
                ctrl_wait_active = 0;
                spin_unlock_irqrestore(&x_lock, f);
                return -5;
            }
        }
        if (cc == EVCC_OK || cc == EVCC_SHORT) {
            if (xfer_len) *xfer_len = got;
            return 0;
        }
        return -5;
    }
}

/* Kesme IN TD kuyrukla (asenkron; tamamlanma poll'de callback'e gider) */
int xhci_intr_submit(uint8_t slot, uint8_t dci, uint8_t* buf, uint32_t len,
                     xhci_intr_cb cb) {
    int d = slot - 1;
    if (!x_present || d < 0 || d >= XHCI_NDEV) return -1;
    struct xhci_devst* ds = &xdev[d];
    if (!ds->slot || ds->dci_intr != dci || ds->intr_pending) return -1;
    if (!buf || !len || len > 4096) return -1;
    uint32_t f;
    spin_lock_irqsave(&x_lock, &f);
    struct xhci_trb* t = &intr_ring[d][ds->intr_idx];
    ds->intr_trb_lo = phys_of(t);
    uint32_t llen = (len > 0x10000u) ? 0x10000u : len;
    trb_write(t, (uint64_t)phys_of(buf), (1u << 17) | llen,
              (XHCI_TRB_NORMAL << 10) | TRB_IOC | ds->intr_cycle);
    ds->intr_idx++;
    if (ds->intr_idx >= XHCI_NCMD) {
        ds->intr_idx = 0;
        ds->intr_cycle ^= 1u;
    }
    ds->intr_pending = 1;
    ds->intr_buf = buf;
    ds->intr_len = len;
    ds->intr_cb = cb;
    w32(x_dboff + (uint32_t)slot * 4u, (uint32_t)dci);
    spin_unlock_irqrestore(&x_lock, f);
    return 0;
}

/* --- port yardımcıları --- */
static uint32_t portsc_rd(int port) {
    return op32(XHCI_PORTSC0 + (uint32_t)port * 0x10u);
}
static void portsc_wr(int port, uint32_t v) {
    opw32(XHCI_PORTSC0 + (uint32_t)port * 0x10u, v);
}

/* Port reset (USB2 protokolü). Dönüş: hız (1/2/3) ya da -1. */
static int xhci_port_reset(int port) {
    uint32_t v = portsc_rd(port);
    if (!(v & PORT_CCS)) return -1;
    /* PP açık tut + PR bas (RWC bitlerini temizle) */
    portsc_wr(port, (v & ~PORT_RWC) | PORT_PP | PORT_PR);
    uint32_t t0 = timer_get_ticks();
    for (;;) {
        uint32_t p = portsc_rd(port);
        if ((p & PORT_PRC) && (p & PORT_PED)) {
            /* değişim bitlerini temizle */
            portsc_wr(port, p);
            uint32_t q = portsc_rd(port);
            return (int)((q >> 10) & 0xFu);
        }
        if ((uint32_t)(timer_get_ticks() - t0) > 100u) return -1;
    }
}

/* --- PCI bul + BAR --- */
static int xhci_pci_find(uint8_t* bus, uint8_t* slot, uint8_t* func) {
    for (uint16_t b = 0; b < 1; b++) {
        for (uint16_t s = 0; s < 32; s++) {
            for (uint16_t f = 0; f < 8; f++) {
                uint32_t v = pci_config_read32((uint8_t)b, (uint8_t)s,
                                               (uint8_t)f, 0x08);
                if (v == 0xFFFFFFFFu) continue;
                uint8_t cl = (uint8_t)(v >> 24);
                uint8_t sub = (uint8_t)(v >> 16);
                uint8_t pro = (uint8_t)(v >> 8);
                if (cl == 0x0C && sub == 0x03 && pro == 0x30) {
                    *bus = (uint8_t)b; *slot = (uint8_t)s; *func = (uint8_t)f;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int xhci_present(void) { return x_present; }

int xhci_init(void) {
    if (x_present) return 0;
    uint8_t bus, slot, func;
    if (xhci_pci_find(&bus, &slot, &func) != 0) {
        serial_puts("[26B] xHCI PCI yok\n");
        return -1;
    }
    /* MEM + bus master (BAR taşıma sonrası tekrar açılır) */
    /* 26B: 64-bit BAR 4GB üstündeyse 32-bit pencereye taşı (OS PCI'yi
     * yönetir; 0xE1000000 QEMU PCI deliğinde boştur). */
    uint32_t bar_lo = pci_config_read32(bus, slot, func, 0x10);
    uint32_t bar_hi = pci_config_read32(bus, slot, func, 0x14);
    serial_puts("[26B] pci "); serial_puthex(bus);
    serial_puts(":"); serial_puthex(slot);
    serial_puts("."); serial_puthex(func);
    serial_puts(" dev="); serial_puthex(pci_config_read32(bus, slot, func, 0));
    serial_puts(" bar0lo="); serial_puthex(bar_lo);
    serial_puts(" bar0hi="); serial_puthex(bar_hi);
    serial_puts("\n");
    if ((bar_lo & 1u) == 0 && ((bar_lo >> 1) & 3u) == 2u && bar_hi != 0) {
        /* 64-bit BAR, 4GB üstü -> 0xE1000000'a taşı */
        uint16_t cmd = pci_config_read16(bus, slot, func, 0x04);
        pci_config_write16(bus, slot, func, 0x04, (uint16_t)(cmd & ~0x02u));
        pci_config_write32(bus, slot, func, 0x10, 0xE1000000u | (bar_lo & 0xFu));
        pci_config_write32(bus, slot, func, 0x14, 0);
        bar_lo = pci_config_read32(bus, slot, func, 0x10);
        bar_hi = pci_config_read32(bus, slot, func, 0x14);
        serial_puts("[26B] BAR tasindi: lo="); serial_puthex(bar_lo);
        serial_puts(" hi="); serial_puthex(bar_hi);
        serial_puts("\n");
    }
    {
        uint16_t cmd = pci_config_read16(bus, slot, func, 0x04);
        cmd |= 0x07u;
        pci_config_write16(bus, slot, func, 0x04, cmd);
    }
    int is_io = 0;
    uint32_t bar0 = bar_lo;
    if (bar0 & 1u) is_io = 1;
    else bar0 &= ~0xFu;
    if (!bar0 || is_io || bar0 >= 0xFEC00000u) {
        serial_puts("[26B] xHCI BAR0 kullanilamaz\n");
        return -1;
    }
    /* BAR boyu sonda */
    uint32_t base = bar0 & ~0xFFFu;
    {
        uint32_t save = pci_config_read32(bus, slot, func, 0x10);
        pci_config_write32(bus, slot, func, 0x10, 0xFFFFFFFFu);
        uint32_t sz = pci_config_read32(bus, slot, func, 0x10);
        pci_config_write32(bus, slot, func, 0x10, save);
        uint32_t mask = sz & ~0xFu;
        uint32_t bytes = (~mask + 1u);
        if (bytes < 0x4000u) bytes = 0x4000u;
        if (bytes > 0x10000u) bytes = 0x10000u;
        for (uint32_t a = base; a < base + bytes; a += 0x1000u)
            paging_map(a, a, PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    }
    x_base = (volatile uint8_t*)(uintptr_t)base;

    /* CAP */
    x_oplen = (uint32_t)(r32(XHCI_CAPLEN) & 0xFFu);
    x_dboff = r32(XHCI_DBOFF) & ~0x3u;
    x_rtsoff = r32(XHCI_RTSOFF) & ~0x1Fu;
    {
        uint32_t p1 = r32(XHCI_HCSP1);
        x_nslots = (uint8_t)(p1 & 0xFFu);
        x_nports = (uint8_t)((p1 >> 24) & 0xFFu);
        uint32_t ver = r32(XHCI_HCIVER);
        uint32_t psz = r32(XHCI_PAGESZ);
        serial_puts("[26B] xHCI bar=0x"); serial_puthex(base);
        serial_puts(" ver="); serial_puthex(ver);
        serial_puts(" slots="); serial_puthex(x_nslots);
        serial_puts(" ports="); serial_puthex(x_nports);
        serial_puts(" pagesz="); serial_puthex(psz);
        serial_puts("\n");
        if (!(psz & 1u)) {
            serial_puts("[26B] 4K sayfa yok\n");
            return -1;
        }
        if (x_nports == 0 || x_nports > 32) {
            serial_puts("[26B] port sayisi garip\n");
            return -1;
        }
    }

    /* LEGSUP: BIOS sahipliğini devral (firmware dokunmuş olabilir).
     * XECP ve next işaretçileri BAR tabanından BAYT offset'idir. */
    {
        uint32_t hccp1 = r32(XHCI_HCCP1);
        uint32_t off = ((hccp1 >> 16) & 0xFFFFu) * 4u;
        int guard = 0;
        while (off && guard++ < 16) {
            uint32_t cap = r32(off);
            uint8_t id = (uint8_t)(cap & 0xFFu);
            uint8_t nxt = (uint8_t)((cap >> 8) & 0xFFu);
            if (id == EXTCAP_LEGSUP) {
                uint32_t sup = r32(off + 4u);
                if (sup & LEGSUP_BIOS_OWNED) {
                    serial_puts("[26B] BIOS sahipligi devraliniyor\n");
                    w32(off + 4u, sup | LEGSUP_OS_OWNED);
                    uint32_t t0 = timer_get_ticks();
                    while (r32(off + 4u) & LEGSUP_BIOS_OWNED) {
                        if ((uint32_t)(timer_get_ticks() - t0) > 100u) break;
                    }
                }
                break;
            }
            off = (uint32_t)nxt;
        }
    }

    /* Durdur + reset */
    {
        uint32_t cmd = op32(XHCI_USBCMD);
        cmd &= ~USBCMD_RS;
        opw32(XHCI_USBCMD, cmd);
        uint32_t t0 = timer_get_ticks();
        while (!(op32(XHCI_USBSTS) & USBSTS_HALT)) {
            if ((uint32_t)(timer_get_ticks() - t0) > 100u) break;
        }
        opw32(XHCI_USBCMD, op32(XHCI_USBCMD) | USBCMD_HCRST);
        t0 = timer_get_ticks();
        while (op32(XHCI_USBCMD) & USBCMD_HCRST) {
            if ((uint32_t)(timer_get_ticks() - t0) > 100u) {
                serial_puts("[26B] HCRST takildi\n");
                return -1;
            }
        }
        t0 = timer_get_ticks();
        while (op32(XHCI_USBSTS) & USBSTS_CNR) {
            if ((uint32_t)(timer_get_ticks() - t0) > 100u) {
                serial_puts("[26B] CNR takildi\n");
                return -1;
            }
        }
    }

    /* DCBAA (scratchpad dahil) */
    for (int i = 0; i < 256; i++) dcbaa[i] = 0;
    /* Scratchpad: HCSPARAMS2 hi[31:27] + lo[25:21] */
    {
        uint32_t p2 = r32(0x08); /* HCSPARAMS2 */
        uint32_t nscratch = (((p2 >> 27) & 0x1Fu) << 5) | ((p2 >> 21) & 0x1Fu);
        if (nscratch > 32u) nscratch = 32u;
        if (nscratch) {
            for (uint32_t i = 0; i < nscratch; i++)
                scratch_arr[i] = (uint64_t)phys_of(scratch_pages[i]);
            dcbaa[0] = (uint64_t)phys_of(scratch_arr);
            serial_puts("[26B] scratchpad="); serial_puthex(nscratch);
            serial_puts("\n");
        }
    }
    opw64(XHCI_DCBAAP, (uint64_t)phys_of(dcbaa));

    /* Komut halkası */
    for (int i = 0; i < XHCI_NCMD; i++) {
        cmd_ring[i].param = 0;
        cmd_ring[i].status = 0;
        cmd_ring[i].control = 0;
    }
    trb_write(&cmd_ring[XHCI_NCMD], (uint64_t)phys_of(cmd_ring),
              0, (XHCI_TRB_LINK << 10) | TRB_TC | 1u);
    cmd_idx = 0;
    cmd_cycle = 1;
    opw64(XHCI_CRCR, (uint64_t)phys_of(cmd_ring) | 1u);

    /* Olay halkası + ERST */
    for (int i = 0; i < XHCI_NEVT; i++) {
        evt_ring[i].param = 0;
        evt_ring[i].status = 0;
        evt_ring[i].control = 0;
    }
    trb_write(&evt_ring[XHCI_NEVT], (uint64_t)phys_of(evt_ring),
              0, (XHCI_TRB_LINK << 10) | TRB_TC | 1u);
    evt_idx = 0;
    evt_cycle = 1;
    erst_arr[0] = phys_of(evt_ring);
    erst_arr[1] = 0;
    erst_arr[2] = XHCI_NEVT;
    erst_arr[3] = 0;
    w32(x_rtsoff + 0x20 + 0x08, 1u); /* ERSTSZ */
    w64(x_rtsoff + 0x20 + 0x10, (uint64_t)phys_of(erst_arr)); /* ERSTBA */
    w64(x_rtsoff + 0x20 + 0x18, (uint64_t)phys_of(evt_ring)); /* ERDP */
    w32(x_rtsoff + 0x20 + 0x04, 0u); /* IMOD */
    w32(x_rtsoff + 0x20, IMAN_IP); /* IP temizle, IE=0 (polling) */

    /* Slot sayısı */
    {
        uint32_t cfg = (x_nslots > 8u) ? 8u : x_nslots;
        if (cfg < 1u) cfg = 1u;
        opw32(XHCI_CONFIG, cfg);
    }
    for (int d = 0; d < XHCI_NDEV; d++) {
        xdev[d].slot = 0;
        xdev[d].dci_intr = 0;
        xdev[d].intr_pending = 0;
    }
    memset(in_ctx, 0, sizeof(in_ctx));

    /* Başlat */
    opw32(XHCI_USBCMD, op32(XHCI_USBCMD) | USBCMD_RS);
    {
        uint32_t t0 = timer_get_ticks();
        while (op32(XHCI_USBSTS) & USBSTS_HALT) {
            if ((uint32_t)(timer_get_ticks() - t0) > 100u) {
                serial_puts("[26B] RS takildi\n");
                return -1;
            }
        }
    }
    /* Portlara güç ver */
    for (int p = 0; p < x_nports; p++) {
        uint32_t v = portsc_rd(p);
        portsc_wr(p, (v & ~PORT_RWC) | PORT_PP);
    }
    x_present = 1;
    serial_puts("[26B] xHCI calisiyor\n");
    return 0;
}

int xhci_selftest(void) {
    if (!x_present) return -1;
    uint32_t sts = op32(XHCI_USBSTS);
    if (sts & USBSTS_HALT) {
        serial_puts("[26B] HALT (calismiyor) [FAIL]\n");
        return -1;
    }
    if (sts & USBSTS_CNR) {
        serial_puts("[26B] CNR [FAIL]\n");
        return -1;
    }
    /* komut halkası nabzı: NOOP gönder */
    {
        uint8_t sl = 0;
        int r = cmd_issue(0, 0, (XHCI_TRB_NOOP_CMD << 10), &sl, 100);
        if (r != 0) {
            serial_puts("[26B] NOOP [FAIL]\n");
            return -1;
        }
    }
    serial_puts("[26B] xhci ports="); serial_puthex(x_nports);
    serial_puts(" slots="); serial_puthex(x_nslots);
    serial_puts(" [PASS]\n");
    return 0;
}

/* --- dışa açık yardımcılar (usbhid kullanır): slot/ep0 tanımları yukarıda --- */

uint8_t* xhci_ctrl_buf(void) { return xfer_data; }
int xhci_port_count(void) { return x_present ? (int)x_nports : 0; }

void xhci_slot_commit(uint8_t slot) {
    int d = (int)slot - 1;
    if (!x_present || d < 0 || d >= XHCI_NDEV) return;
    dcbaa[slot] = (uint64_t)phys_of(dev_ctx[d]);
}

/* Configure input ctx'ini mevcut output ctx'ten başlat (EDK2 yöntemi).
 * Sıfır EP0 ile Configure TRB Error (cc=5) veriyordu. */
int xhci_cfg_begin(uint8_t slot, uint32_t* ictx, uint8_t dci_new) {
    int d = (int)slot - 1;
    if (!x_present || d < 0 || d >= XHCI_NDEV || !ictx) return -1;
    if (dci_new < 2 || dci_new > 31) return -1;
    uint32_t f;
    spin_lock_irqsave(&x_lock, &f);
    memset(ictx, 0, 33u * 8u * 4u);
    /* 26C-DÜZEN DÜZELTMESİ: device ctx'te slot dw0-7 + EP0 dw8-15;
     * input ctx'te slot dw8-15 + EP0 dw16-23. Karışmıştı! */
    for (int i = 0; i < 16; i++) ictx[8 + i] = dev_ctx[d][i];
    /* entries güncelle */
    ictx[8] = (ictx[8] & ~(0x1Fu << 27)) | ((uint32_t)dci_new << 27);
    ictx[0] = 0;
    ictx[1] = (1u << dci_new);
    spin_unlock_irqrestore(&x_lock, f);
    return 0;
}
int xhci_port_reset_pub(int port) {
    if (!x_present || port < 0 || port >= (int)x_nports) return -1;
    return xhci_port_reset(port);
}
