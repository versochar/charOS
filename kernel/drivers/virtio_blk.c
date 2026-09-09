#include <drivers/virtio_blk.h>
#include <drivers/pci.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 13G: virtio-blk legacy (transitional) sürücü.
 * QEMU: -device virtio-blk-pci-transitional -> I/O BAR üzerinden
 * legacy register seti. Polling ile tamamlanma beklenir (IRQ yok). */

/* Legacy register offsetleri (I/O BAR tabanı +) */
#define VDEV_FEAT   0x00  /* u32 R: device features */
#define VGUEST_FEAT 0x04  /* u32 W: guest features */
#define VQ_PFN      0x08  /* u32 W: queue pfn */
#define VQ_NUM_MAX  0x0C  /* u16 R: max queue size */
#define VQ_SEL      0x0E  /* u16 W: queue select */
#define VQ_NOTIFY   0x10  /* u16 W: queue notify */
#define VDEV_STATUS 0x12  /* u8  RW: device status */
#define VISR        0x13  /* u8  R: isr status (okuma temizler) */
#define VCONFIG     0x14  /* cihaz config (blk: u64 capacity) */

/* Status bitleri */
#define ST_ACK       0x01
#define ST_DRIVER    0x02
#define ST_DRIVER_OK 0x04
#define ST_FEAT_OK   0x08
#define ST_FAILED    0x80

/* Request tipleri / sonuç */
#define BLK_T_IN   0
#define BLK_T_OUT  1
#define BLK_S_OK   0

/* Descriptor flagleri */
#define DESC_F_NEXT  1
#define DESC_F_WRITE 2

#define VQ_SIZE 256  /* legacy: halka boyu = QueueNumMax (QEMU blk: 256) */

struct vring_desc {
    uint32_t addr_lo;
    uint32_t addr_hi;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct vring_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VQ_SIZE];
} __attribute__((packed));

struct vring_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct vring_used {
    uint16_t flags;
    uint16_t idx;
    struct vring_used_elem ring[VQ_SIZE];
} __attribute__((packed));

struct blk_req {
    uint32_t type;
    uint32_t reserved;
    uint32_t sector_lo;
    uint32_t sector_hi;
} __attribute__((packed));

/* DMA alanları: identity-map'li düşük bellek (BSS), virt==phys.
 * Legacy halka boyu = QueueNumMax (256): desc 4K @+0, avail ~516B @+4096,
 * used ~2K @+8192 (used 4096-hizalı şart). Toplam 12KB. */
static uint8_t vring_area[3 * 4096] __attribute__((aligned(4096)));
static struct blk_req blk_header __attribute__((aligned(16)));
static uint8_t blk_data[VIRTIO_SECTOR_SIZE] __attribute__((aligned(16)));
static volatile uint8_t blk_status;

static struct vring_desc*  vdesc;
static struct vring_avail* vavail;
static struct vring_used*  vused;

static uint16_t vio_base = 0;      /* I/O BAR tabanı */
static int vio_present = 0;
static uint32_t vio_capacity = 0;  /* sektör sayısı */
static uint16_t vio_last_used = 0;
static spinlock_t vio_lock = SPINLOCK_INIT;

static void mb(void) { asm volatile("mfence" ::: "memory"); }

static uint8_t vstat(void) { return pci_inb(vio_base + VDEV_STATUS); }

static int vio_request(uint32_t type, uint32_t lba, int is_write) {
    /* desc0: header (device okur) */
    blk_header.type = type;
    blk_header.reserved = 0;
    blk_header.sector_lo = lba;
    blk_header.sector_hi = 0;
    blk_status = 0xFF;

    vdesc[0].addr_lo = (uint32_t)&blk_header; vdesc[0].addr_hi = 0;
    vdesc[0].len = sizeof(blk_header);
    vdesc[0].flags = DESC_F_NEXT; vdesc[0].next = 1;

    vdesc[1].addr_lo = (uint32_t)blk_data; vdesc[1].addr_hi = 0;
    vdesc[1].len = VIRTIO_SECTOR_SIZE;
    vdesc[1].flags = DESC_F_NEXT | (is_write ? 0 : DESC_F_WRITE);
    vdesc[1].next = 2;

    vdesc[2].addr_lo = (uint32_t)&blk_status; vdesc[2].addr_hi = 0;
    vdesc[2].len = 1;
    vdesc[2].flags = DESC_F_WRITE; vdesc[2].next = 0;

    mb();
    vavail->ring[vavail->idx % VQ_SIZE] = 0;
    mb();
    vavail->idx++;
    mb();
    vavail->flags = 0;
    pci_outw(vio_base + VQ_NOTIFY, 0); /* kuyruk 0 */

    /* Tamamlanmayı poll ile bekle (kilitlenme önleyici timeout'lu) */
    int iters = 0;
    for (volatile int i = 0; i < 20000000; i++) {
        mb();
        if (vused->idx != vio_last_used) break;
        if ((i & 0xFFF) == 0) asm volatile("pause");
        iters = i;
    }
    mb();
    if (vused->idx == vio_last_used) {
        serial_puts("[13G] blk timeout lba="); serial_puthex(lba);
        serial_puts(" status="); serial_puthex(vstat());
        serial_puts(" isr="); serial_puthex(pci_inb(vio_base + VISR));
        serial_puts(" used="); serial_puthex(vused->idx);
        serial_puts(" last="); serial_puthex(vio_last_used);
        serial_puts(" avail="); serial_puthex(vavail->idx);
        serial_puts(" iters="); serial_puthex(iters);
        serial_puts(" qmax="); serial_puthex(pci_inw(vio_base + VQ_NUM_MAX));
        serial_puts("\n");
        return -1;
    }
    vio_last_used = vused->idx;
    (void)pci_inb(vio_base + VISR); /* ISR temizle */
    if (blk_status != BLK_S_OK) {
        serial_puts("[13G] blk status!=OK st="); serial_puthex(blk_status); serial_puts("\n");
        return -1;
    }
    return 0;
}

int virtio_blk_present(void) { return vio_present; }
uint32_t virtio_blk_capacity(void) { return vio_capacity; }

int block_read_sector(uint32_t lba, void* buf) {
    if (!vio_present || !buf) return -1;
    if (lba >= vio_capacity) return -1;
    uint32_t flags;
    spin_lock_irqsave(&vio_lock, &flags);
    int r = vio_request(BLK_T_IN, lba, 0);
    if (r == 0) memcpy(buf, blk_data, VIRTIO_SECTOR_SIZE);
    spin_unlock_irqrestore(&vio_lock, flags);
    return r;
}

int block_write_sector(uint32_t lba, const void* buf) {
    if (!vio_present || !buf) return -1;
    if (lba >= vio_capacity) return -1;
    uint32_t flags;
    spin_lock_irqsave(&vio_lock, &flags);
    memcpy(blk_data, buf, VIRTIO_SECTOR_SIZE);
    int r = vio_request(BLK_T_OUT, lba, 1);
    spin_unlock_irqrestore(&vio_lock, flags);
    return r;
}

int virtio_blk_init(void) {
    uint8_t bus = 0, slot = 0, func = 0;
    /* Önce legacy ID (0x1001), sonra transitional (0x1042) dene */
    if (pci_find_device(0x1AF4, 0x1001, &bus, &slot, &func) != 0) {
        if (pci_find_device(0x1AF4, 0x1042, &bus, &slot, &func) != 0) {
            serial_puts("[13G] virtio-blk PCI cihazı yok\n");
            vga_puts("[13G] virtio-blk not found\n");
            return -1;
        }
    }
    serial_puts("[13G] virtio-blk @ "); serial_puthex(slot);
    serial_puts("."); serial_puthex(func); serial_puts("\n");

    pci_enable_io_bm(bus, slot, func);
    {
        uint16_t cmd = pci_config_read16(bus, slot, func, 0x04);
        serial_puts("[13G] PCI cmd=0x"); serial_puthex(cmd); serial_puts("\n");
    }

    /* I/O BAR bul */
    uint16_t iobase = 0;
    for (int b = 0; b < 6; b++) {
        int is_io = 0;
        uint32_t addr = pci_read_bar(bus, slot, func, b, &is_io);
        if (is_io && addr) { iobase = (uint16_t)addr; break; }
    }
    if (!iobase) {
        serial_puts("[13G] virtio-blk I/O BAR yok\n");
        return -1;
    }
    vio_base = iobase;
    serial_puts("[13G] virtio-blk iobase=0x"); serial_puthex(iobase); serial_puts("\n");

    /* Status handshake */
    pci_outb(vio_base + VDEV_STATUS, 0);            /* reset */
    pci_outb(vio_base + VDEV_STATUS, ST_ACK);
    pci_outb(vio_base + VDEV_STATUS, ST_ACK | ST_DRIVER);
    (void)pci_inl(vio_base + VDEV_FEAT);            /* features oku */
    pci_outl(vio_base + VGUEST_FEAT, 0);            /* ek özellik yok */
    pci_outb(vio_base + VDEV_STATUS, ST_ACK | ST_DRIVER | ST_FEAT_OK);
    if (!(vstat() & ST_FEAT_OK)) {
        serial_puts("[13G] features rejected\n");
        pci_outb(vio_base + VDEV_STATUS, ST_FAILED);
        return -1;
    }

    /* Kuyruk 0 kur (legacy boy = qmax) */
    pci_outw(vio_base + VQ_SEL, 0);
    uint16_t qmax = pci_inw(vio_base + VQ_NUM_MAX);
    serial_puts("[13G] queue max="); serial_puthex(qmax); serial_puts("\n");
    if (qmax == 0 || qmax > VQ_SIZE) {
        serial_puts("[13G] queue boyu uygun değil\n");
        return -1;
    }
    vdesc  = (struct vring_desc*)(vring_area + 0);
    vavail = (struct vring_avail*)(vring_area + 4096);
    vused  = (struct vring_used*)(vring_area + 8192);
    memset(vring_area, 0, sizeof(vring_area));
    vavail->flags = 0;
    vused->flags = 0;
    vio_last_used = 0;
    pci_outl(vio_base + VQ_PFN, ((uint32_t)vring_area) >> 12);
    serial_puts("[13G] vring=0x"); serial_puthex((uint32_t)vring_area);
    serial_puts(" pfn=0x"); serial_puthex(((uint32_t)vring_area) >> 12);
    serial_puts("\n");

    /* Kapasite (config+0, u64 sektör) */
    uint32_t cap_lo = pci_inl(vio_base + VCONFIG + 0);
    uint32_t cap_hi = pci_inl(vio_base + VCONFIG + 4);
    (void)cap_hi;
    vio_capacity = cap_lo;
    serial_puts("[13G] capacity="); serial_puthex(vio_capacity); serial_puts(" sectors\n");
    vga_puts("[13G] virtio-blk cap="); vga_putdec(vio_capacity); vga_puts(" sec\n");

    pci_outb(vio_base + VDEV_STATUS, ST_ACK | ST_DRIVER | ST_FEAT_OK | ST_DRIVER_OK);
    if (!(vstat() & ST_DRIVER_OK)) {
        serial_puts("[13G] DRIVER_OK reddedildi\n");
        return -1;
    }
    vio_present = 1;
    serial_puts("[13G] virtio-blk hazır\n");
    return 0;
}

/* Scratch sektör = son sektör (13H FS'i LBA0'dan başlar, çakışmaz) */
int virtio_blk_selftest(void) {
    if (!vio_present || vio_capacity < 8) {
        serial_puts("[13G] selftest atlandı (cihaz yok)\n");
        return -1;
    }
    static uint8_t wbuf[VIRTIO_SECTOR_SIZE];
    static uint8_t rbuf[VIRTIO_SECTOR_SIZE];
    uint32_t lba = vio_capacity - 1;
    for (int i = 0; i < VIRTIO_SECTOR_SIZE; i++) wbuf[i] = (uint8_t)(0xA5 + (i & 0xFF));
    memset(rbuf, 0, sizeof(rbuf));
    if (block_write_sector(lba, wbuf) != 0) {
        serial_puts("[13G] selftest WRITE [FAIL]\n");
        vga_puts("[13G] selftest WRITE [FAIL]\n");
        return -1;
    }
    if (block_read_sector(lba, rbuf) != 0) {
        serial_puts("[13G] selftest READ [FAIL]\n");
        return -1;
    }
    if (memcmp(wbuf, rbuf, VIRTIO_SECTOR_SIZE) != 0) {
        serial_puts("[13G] selftest COMPARE [FAIL]\n");
        vga_puts("[13G] selftest COMPARE [FAIL]\n");
        return -1;
    }
    serial_puts("[13G] virtio-blk sector RW [PASS]\n");
    vga_puts("[13G] virtio-blk sector RW [PASS]\n");
    return 0;
}
