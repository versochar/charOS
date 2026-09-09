#include <net/e1000.h>
#include <net/net.h>
#include <drivers/pci.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <string.h>
#include <core/spinlock.h>

/* 21A: Gerçek e1000 (82540EM) sürücü — QEMU'nun -device e1000 bileşeni.
 * MMIO BAR0 üzerinden register erişimi; TX/RX DMA halkaları identity
 * map'li statik tamponlarda (düşük fiziksel bellek). Polling modeli:
 * - Gelen paketler timer IRQ0 (timer_isr) içinden e1000_poll ile alınır.
 * - Gönderim e1000_send ile TDT ilerletilir, DD biti beklenir.
 * 82540EM (0x100E): RX/TX tanımlayıcıları 16 bayt legacy formattır. */

#define E1000_CTRL     0x0000
#define E1000_STATUS   0x0008
#define E1000_ICR      0x00C0
#define E1000_IMS      0x00D0
#define E1000_RCTL     0x0100
#define E1000_RDBAL    0x2800
#define E1000_RDBAH    0x2804
#define E1000_RDLEN    0x2808
#define E1000_RDH      0x2810
#define E1000_RDT      0x2818
#define E1000_RDTR     0x2820
#define E1000_RXDCTL   0x2828
#define E1000_TCTL     0x0400
#define E1000_TDBAL    0x3800
#define E1000_TDBAH    0x3804
#define E1000_TDLEN    0x3808
#define E1000_TDH      0x3810
#define E1000_TDT      0x3818
#define E1000_TIPG     0x0410
#define E1000_MTA      0x5200
#define E1000_RA_BASE  0x5400

#define E1000_CTRL_RST    (1u << 26)
#define E1000_CTRL_SLU    (1u << 6)
#define E1000_CTRL_LRST   (1u << 3)
#define E1000_CTRL_ASDE   (1u << 5)

#define RCTL_EN     (1u << 0)
#define RCTL_SBP    (1u << 1)
#define RCTL_UPE    (1u << 2)
#define RCTL_MPE    (1u << 3)
#define RCTL_BAM    (1u << 15)
#define RCTL_SECRC  (1u << 26)
#define RCTL_BSIZE_2K (0u << 16)  /* 2048 bayt rx tamponu */

#define TCTL_EN     (1u << 0)
#define TCTL_PSP    (1u << 1)
#define TCTL_CT_15  (0x0Fu << 4)
#define TCTL_COLD   (0x40u << 12)

#define TIPG_IPGT   (0x0Au << 0)
#define TIPG_IPGR1  (0x20u << 10)
#define TIPG_IPGR2  (0x20u << 20)

#define TD_CMD_EOP  (1u << 0)
#define TD_CMD_IFCS (1u << 1)
#define TD_CMD_RS   (1u << 3)
#define TD_STAT_DD  (1u << 0)

#define RD_STAT_DD  (1u << 0)
#define RD_STAT_EOP (1u << 1)

#define NIC_RX_DESC 32
#define NIC_TX_DESC 32
#define NIC_BUF_SIZE 2048

struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

/* DMA alanları: fiziksel adres == sanal (identity, düşük bellek) */
static struct e1000_tx_desc txq[NIC_TX_DESC] __attribute__((aligned(4096)));
static struct e1000_rx_desc rxq[NIC_RX_DESC] __attribute__((aligned(4096)));
static uint8_t tx_bufs[NIC_TX_DESC][NIC_BUF_SIZE] __attribute__((aligned(4096)));
static uint8_t rx_bufs[NIC_RX_DESC][NIC_BUF_SIZE] __attribute__((aligned(4096)));

static volatile uint32_t* e_regs = 0;   /* MMIO BAR0 sanal adresi */
static int e_present = 0;
static int tx_tail = 0;
static int rx_cur = 0;
static uint8_t e_mac[6];
static int e_link_ok = 0;

static uint32_t e_read(uint32_t off) {
    return *(volatile uint32_t*)((volatile uint8_t*)e_regs + (off & ~3u));
}
static void e_write(uint32_t off, uint32_t v) {
    *(volatile uint32_t*)((volatile uint8_t*)e_regs + (off & ~3u)) = v;
}

/* EEPROM (EERD) okuma: veri döngüsü DO biti oturana kadar bekle. */
static uint16_t eeprom_read(uint16_t addr) {
    for (int t = 0; t < 1000; t++) {
        e_write(0x0014, addr << 8 | 1u); /* EERD: start */
        if (e_read(0x0014) & (1u << 4))   /* DO değer hazır */
            return (uint16_t)(e_read(0x0014) >> 16);
    }
    return 0;
}

int e1000_init(void) {
    if (e_present) return 0;
    uint8_t bus = 0, slot = 0, func = 0;
    /* QEMU -device e1000 -> 82540EM (0x100E). Fallback 82545 (0x100F). */
    if (pci_find_device(0x8086, 0x100E, &bus, &slot, &func) != 0) {
        if (pci_find_device(0x8086, 0x100F, &bus, &slot, &func) != 0) {
            serial_puts("[21A] e1000 PCI yok\n");
            vga_puts("[8B] e1000 not found\n");
            return -1;
        }
    }
    uint32_t cmd = pci_config_read16(bus, slot, func, 0x04);
    cmd |= 0x7u; /* IO + memory + bus master */
    pci_config_write16(bus, slot, func, 0x04, cmd);

    int is_io = 0;
    uint32_t bar0 = pci_read_bar(bus, slot, func, 0, &is_io);
    if (!bar0 || is_io) {
        serial_puts("[21A] e1000 BAR0 MMIO değil\n");
        return -1;
    }
    /* MMIO identity map — BAR0 128KB register bloğu (RA=0x5400'a kadar). */
    uint32_t base = bar0 & ~0xFFFu;
    for (uint32_t a = base; a < base + 0x20000u; a += 0x1000u)
        paging_map(a, a, PAGE_PRESENT | PAGE_RW);
    e_regs = (volatile uint32_t*)bar0;
    e_present = 1;

    /* Reset + link up */
    e_write(E1000_CTRL, 0);
    e_write(E1000_CTRL, E1000_CTRL_RST);
    volatile uint32_t w;
    for (w = 0; w < 100000; w++) { if (!(e_read(E1000_CTRL) & E1000_CTRL_RST)) break; }
    e_write(E1000_CTRL, E1000_CTRL_SLU);

    /* MAC: EEPROM 0,1,2 (48 bit) */
    uint16_t w0 = eeprom_read(0);
    uint16_t w1 = eeprom_read(1);
    uint16_t w2 = eeprom_read(2);
    if (w0 == 0 && w1 == 0 && w2 == 0) {
        /* QEMU varsayılan MAC (RA kaydından) */
        e_mac[0] = 0x52; e_mac[1] = 0x54; e_mac[2] = 0x00;
        e_mac[3] = 0x12; e_mac[4] = 0x34; e_mac[5] = 0x56;
    } else {
        e_mac[0] = w0 & 0xFF; e_mac[1] = w0 >> 8;
        e_mac[2] = w1 & 0xFF; e_mac[3] = w1 >> 8;
        e_mac[4] = w2 & 0xFF; e_mac[5] = w2 >> 8;
    }

    /* RX halkası */
    memset(rxq, 0, sizeof(rxq));
    e_write(E1000_RDBAL, (uint32_t)(uintptr_t)rxq & 0xFFFFFFFFu);
    e_write(E1000_RDBAH, 0);
    e_write(E1000_RDLEN, sizeof(rxq));
    e_write(E1000_RDH, 0);
    for (int i = 0; i < NIC_RX_DESC; i++)
        rxq[i].addr = (uint64_t)(uintptr_t)rx_bufs[i];
    rx_cur = 0;
    /* MTA (multicast) temizle */
    for (int i = 0; i < 128; i++) e_write(E1000_MTA + i * 4, 0);
    /* RA0 = MAC (unicast alım) */
    e_write(E1000_RA_BASE,     (uint32_t)(e_mac[0]) | ((uint32_t)e_mac[1] << 8) |
                               ((uint32_t)e_mac[2] << 16) | ((uint32_t)e_mac[3] << 24));
    e_write(E1000_RA_BASE + 4, (uint32_t)e_mac[4] | ((uint32_t)e_mac[5] << 8) | (1u << 31));
    e_write(E1000_RDT, NIC_RX_DESC - 1);
    e_write(E1000_RDTR, 0);  /* poll, DTE yok */
    e_write(E1000_RXDCTL, 0);
    e_write(E1000_RCTL, RCTL_EN | RCTL_BAM | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_BSIZE_2K);

    /* TX halkası */
    memset(txq, 0, sizeof(txq));
    e_write(E1000_TDBAL, (uint32_t)(uintptr_t)txq & 0xFFFFFFFFu);
    e_write(E1000_TDBAH, 0);
    e_write(E1000_TDLEN, sizeof(txq));
    e_write(E1000_TDH, 0);
    e_write(E1000_TDT, 0);
    tx_tail = 0;
    e_write(E1000_TIPG, TIPG_IPGT | TIPG_IPGR1 | TIPG_IPGR2);
    e_write(E1000_TCTL, TCTL_EN | TCTL_PSP | TCTL_CT_15 | TCTL_COLD);

    /* 26A: RX yerleşmesi — RCTL.EN sonrası QEMU ~1000ms kuyrukta tutar
     * (flush penceresi) + gerçek link settle. Aynı tick saatiyle 110 tick
     * beklenir; arka planda timer IRQ poll'lamaya devam eder. */
    timer_wait(110);

    /* Link durumu */
    e_link_ok = (e_read(E1000_STATUS) & (1u << 1)) ? 1 : 0;
    {
        serial_puts("[21A] e1000 MAC=");
        for (int i = 0; i < 6; i++) { serial_puthex(e_mac[i]); if (i < 5) serial_puts(":"); }
        serial_puts(" link="); serial_puthex((uint32_t)e_link_ok);
        serial_puts(" bar=0x"); serial_puthex(bar0);
        serial_puts("\n");
    }
    return 0;
}

int e1000_present(void) { return e_present; }

void e1000_mac_get(uint8_t* out) {
    if (!out) return;
    for (int i = 0; i < 6; i++) out[i] = e_mac[i];
}

int e1000_link(void) { return e_link_ok; }

int e1000_send(const uint8_t* data, int len) {
    if (!e_present || !data || len <= 0) return -1;
    if (len > NIC_BUF_SIZE) len = NIC_BUF_SIZE;
    int idx = tx_tail;
    /* önceki desc hâlâ meşgulse dön */
    if (txq[idx].status & TD_STAT_DD) return -1;
    if (txq[idx].cmd) return -1;
    memcpy(tx_bufs[idx], data, len);
    txq[idx].addr = (uint64_t)(uintptr_t)tx_bufs[idx];
    txq[idx].length = (uint16_t)len;
    txq[idx].cmd = TD_CMD_EOP | TD_CMD_IFCS | TD_CMD_RS;
    txq[idx].status = 0;
    e_write(E1000_TDT, (uint32_t)(idx + 1) % NIC_TX_DESC);
    /* DD bitini bekle */
    for (volatile int i = 0; i < 5 * 1000 * 1000; i++) {
        if (txq[idx].status & TD_STAT_DD) break;
    }
    tx_tail = (idx + 1) % NIC_TX_DESC;
    return len;
}

/* Gelen paketleri ring'den al (timer IRQ + ana döngüden çağrılır).
 * 26A: SMP kilidi — IRQ0 (CPU0) ile test döngüsü (başka CPU) aynı anda
 * girebiliyordu; rx_cur kayıp paket düşürüyordu (UEFI'de ARP flakes). */
static spinlock_t poll_lock = SPINLOCK_INIT;

void e1000_poll(void) {
    uint32_t irqfl;
    if (!e_present) return;
    spin_lock_irqsave(&poll_lock, &irqfl);
    while (1) {
        int idx = rx_cur;
        if (!(rxq[idx].status & RD_STAT_DD)) break;
        int n = rxq[idx].length;
        if (n > NIC_BUF_SIZE) n = NIC_BUF_SIZE;
        if (n > 0 && (rxq[idx].status & RD_STAT_EOP))
            net_handle_packet(rx_bufs[idx], n);
        rxq[idx].status = 0;
        e_write(E1000_RDT, (uint32_t)((idx + NIC_RX_DESC - 1) % NIC_RX_DESC));
        rx_cur = (idx + 1) % NIC_RX_DESC;
    }
    spin_unlock_irqrestore(&poll_lock, irqfl);
}

/* Timer hook: IRQ0 içinden poll. ISR senkronizasyonu single-CPU varsayımı
 * (timer periyodu uzay olduğundan çakışma riski ihmal edilir). */
void e1000_timer_poll(void) { e1000_poll(); }

void e1000_receive(void) { e1000_poll(); }