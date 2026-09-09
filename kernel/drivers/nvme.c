#include <drivers/nvme.h>
#include <drivers/pci.h>
#include <drivers/serial.h>
#include <drivers/vga.h>
#include <drivers/timer.h>
#include <core/spinlock.h>
#include <memory/paging.h>
#include <string.h>

/* 27A: NVMe 1.3 host sürücüsü (polling, 1 I/O kuyruk çifti).
 * 27B: I/O okuma/yazma + selftest.
 * DMA: statik 4K-hizalı düşük bellek tamponları (identity, virt==phys).
 * Zaman aşımı: PIT 100Hz tick'leri. */

/* --- register offsetleri (BAR0 +) --- */
#define R_CAP     0x00  /* u64 */
#define R_VS      0x08  /* u32 */
#define R_INTMS   0x0C  /* u32 W */
#define R_CC      0x14  /* u32 */
#define R_CSTS    0x1C  /* u32 */
#define R_AQA     0x24  /* u32 */
#define R_ASQ     0x28  /* u64 */
#define R_ACQ     0x30  /* u64 */
#define R_DB      0x1000 /* doorbell tabanı */

/* CC bitleri */
#define CC_EN     (1u << 0)
#define CC_CSS_NVM (0u << 1)
#define CC_MPS_4K (0u << 4)
#define CC_AMS_RR (0u << 7)
#define CC_SHN_NONE (0u << 11)
#define CC_IOSQES_64 (6u << 16)
#define CC_IOCQES_16 (4u << 20)

/* CSTS */
#define CSTS_RDY  (1u << 0)
#define CSTS_CFS  (1u << 1)

/* Admin opcode'lar */
#define A_IDENTIFY   0x06
#define A_GETFEAT    0x0A
#define A_SETFEAT    0x09
#define A_CRCQ       0x05
#define A_CRSQ       0x01
#define A_DELQ       0x00
#define A_DELQC      0x04

/* I/O opcode'lar */
#define IO_READ      0x02
#define IO_WRITE     0x01

/* CNS */
#define CNS_NS       0x00
#define CNS_CTRL     0x01
#define CNS_NSLIST   0x02

/* Feature */
#define F_NUMQ       0x07

struct sqe {
    uint32_t cdw0;
    uint32_t nsid;
    uint32_t cdw2, cdw3;
    uint64_t mptr;
    uint64_t prp1, prp2;
    uint32_t cdw10, cdw11, cdw12, cdw13, cdw14, cdw15;
} __attribute__((packed));

struct cqe {
    uint32_t dw0, dw1;
    uint16_t sqhd, sqid;
    uint16_t cid, status;
} __attribute__((packed));

#define ASIZE 64    /* admin SQ/CQ girişi */
#define ISIZE 128   /* I/O SQ/CQ girişi */

static uint8_t n_bus, n_slot, n_func;
static volatile uint8_t* n_bar;
static uint32_t n_dstrd;      /* doorbell stride shift (4<<dstrd) */
static int n_present;

static struct sqe asq[ASIZE] __attribute__((aligned(4096)));
static struct cqe acq[ASIZE] __attribute__((aligned(4096)));
static struct sqe isq[ISIZE] __attribute__((aligned(4096)));
static struct cqe icq[ISIZE] __attribute__((aligned(4096)));
static uint8_t ident_buf[4096] __attribute__((aligned(4096)));
static uint8_t data_buf[4096] __attribute__((aligned(4096)));

static uint16_t a_tail, a_head;
static uint8_t a_phase;
static uint16_t i_tail, i_head;
static uint8_t i_phase;
static uint16_t next_cid = 1;

static uint32_t nsid0;        /* ilk aktif namespace */
static uint32_t lba_shift;    /* 512B sektör -> yerel LBA kaydırma (4K LBA: 3) */
static uint32_t lba_bytes;    /* yerel LBA boyutu */
static uint64_t ns_blocks;    /* namespace yerel blok sayısı */
static spinlock_t n_lock = SPINLOCK_INIT;

static uint32_t phys_of(const void* p) { return (uint32_t)(uintptr_t)p; }

static uint32_t r32(uint32_t off) {
    return *(volatile uint32_t*)(n_bar + off);
}
static void w32(uint32_t off, uint32_t v) {
    *(volatile uint32_t*)(n_bar + off) = v;
}
static uint64_t r64(uint32_t off) {
    uint64_t lo = r32(off), hi = r32(off + 4);
    return lo | (hi << 32);
}
static void w64(uint32_t off, uint64_t v) {
    w32(off, (uint32_t)v);
    w32(off + 4, (uint32_t)(v >> 32));
}
static void wdb(uint32_t qid, int is_cq, uint16_t val) {
    /* SQ tail: 0x1000+2*qid*step ; CQ head: 0x1000+(2*qid+1)*step */
    uint32_t step = 4u << n_dstrd;
    uint32_t off = R_DB + (2u * qid + (is_cq ? 1u : 0u)) * step;
    w32(off, val);
}
static void mb(void) { asm volatile("mfence" ::: "memory"); }

/* Kuyrukta komut çalıştır (admin=0 / io=1). cqe_out doldurulur.
 * 0 başarı (SC=0), -1 hata/zaman aşımı. KİLİT DIŞINDA da çağrılabilir
 * ama I/O yolu kilitler; admin init'te tek iş parçacığı varsayılır. */
static int run_cmd(int is_io, const struct sqe* s, struct cqe* cqe_out) {
    struct sqe* sq;
    struct cqe* cq;
    uint16_t size, tail, head, *ptail, *phead;
    uint8_t* pphase;
    uint32_t qid = is_io ? 1u : 0u;
    if (is_io) { sq = isq; cq = icq; size = ISIZE; ptail = &i_tail; phead = &i_head; pphase = &i_phase; }
    else       { sq = asq; cq = acq; size = ASIZE; ptail = &a_tail; phead = &a_head; pphase = &a_phase; }

    tail = *ptail; head = *phead;
    sq[tail] = *s;
    mb();
    tail = (uint16_t)((tail + 1u) % size);
    *ptail = tail;
    wdb(qid, 0, tail); /* SQ tail doorbell */

    /* Tamamlanmayı poll ile bekle (~3 sn) */
    uint32_t t0 = timer_get_ticks();
    for (;;) {
        mb();
        uint16_t st = cq[head].status;
        if ((uint8_t)(st & 1u) == *pphase) break;
        if ((uint32_t)(timer_get_ticks() - t0) > 300u) {
            serial_puts("[27A] komut timeout q="); serial_puthex(qid);
            serial_puts(" cid="); serial_puthex((s->cdw0 >> 16) & 0xFFFFu);
            serial_puts("\n");
            return -1;
        }
    }
    *cqe_out = cq[head];
    mb();
    head = (uint16_t)((head + 1u) % size);
    if (head == 0) *pphase ^= 1u;
    *phead = head;
    wdb(qid, 1, head); /* CQ head doorbell */

    uint16_t st = cqe_out->status;
    uint32_t sc = (st >> 1) & 0xFFu;
    uint32_t sct = (st >> 9) & 0x7u;
    if (sc != 0 || sct != 0) {
        serial_puts("[27A] komut SC="); serial_puthex(sc);
        serial_puts(" SCT="); serial_puthex(sct);
        serial_puts(" op="); serial_puthex((s->cdw0 & 0xFFu));
        serial_puts(" cid="); serial_puthex((s->cdw0 >> 16) & 0xFFFFu);
        serial_puts("\n");
        return -1;
    }
    return 0;
}

static uint16_t new_cid(void) {
    uint16_t c = next_cid++;
    if (next_cid == 0) next_cid = 1;
    return c;
}

/* Identify (CNS, nsid) -> ident_buf. 0 başarı. */
static int identify(uint32_t cns, uint32_t nsid) {
    struct sqe s;
    struct cqe c;
    memset(&s, 0, sizeof(s));
    s.cdw0 = (uint32_t)A_IDENTIFY | ((uint32_t)new_cid() << 16);
    s.nsid = nsid;
    s.prp1 = phys_of(ident_buf);
    s.cdw10 = cns;
    memset(ident_buf, 0, sizeof(ident_buf));
    mb();
    return run_cmd(0, &s, &c);
}

/* Get Features fid -> *res. 0 başarı. */
static int getfeat(uint32_t fid, uint32_t nsid, uint32_t* res) {
    struct sqe s;
    struct cqe c;
    memset(&s, 0, sizeof(s));
    s.cdw0 = (uint32_t)A_GETFEAT | ((uint32_t)new_cid() << 16);
    s.nsid = nsid;
    s.cdw10 = fid;
    if (run_cmd(0, &s, &c) != 0) return -1;
    *res = c.dw0;
    return 0;
}

/* I/O kuyruk çifti kur (qid=1). 0 başarı. */
static int create_ioq(void) {
    struct sqe s;
    struct cqe c;
    uint32_t qsize = ISIZE; /* 0-tabanlı alan için -1 */

    /* Önce CQ (PC=1, IEN=0 polling, IV=0) */
    memset(&s, 0, sizeof(s));
    s.cdw0 = (uint32_t)A_CRCQ | ((uint32_t)new_cid() << 16);
    s.prp1 = phys_of(icq);
    s.cdw10 = (1u) | ((qsize - 1u) << 16); /* QID=1, QSIZE */
    s.cdw11 = (1u << 0); /* PC=1 */
    for (int i = 0; i < ISIZE; i++) { icq[i].dw0 = icq[i].dw1 = 0; icq[i].sqhd = icq[i].sqid = 0; icq[i].cid = 0; icq[i].status = 0; }
    mb();
    if (run_cmd(0, &s, &c) != 0) {
        serial_puts("[27A] IOCQ kurulumu basarisiz\n");
        return -1;
    }
    /* Sonra SQ (PC=1, CQID=1) */
    memset(&s, 0, sizeof(s));
    s.cdw0 = (uint32_t)A_CRSQ | ((uint32_t)new_cid() << 16);
    s.prp1 = phys_of(isq);
    s.cdw10 = (1u) | ((qsize - 1u) << 16);
    s.cdw11 = (1u << 0) | (1u << 16); /* PC=1, CQID=1 */
    for (int i = 0; i < ISIZE; i++) memset(&isq[i], 0, sizeof(isq[i]));
    mb();
    if (run_cmd(0, &s, &c) != 0) {
        serial_puts("[27A] IOSQ kurulumu basarisiz\n");
        return -1;
    }
    i_tail = i_head = 0;
    i_phase = 1;
    return 0;
}

int nvme_present(void) { return n_present; }

int nvme_init(void) {
    /* PCI class 01:08:02 (NVMe) tara - bus 0 */
    int found = 0;
    for (uint8_t s = 0; s < 32 && !found; s++) {
        uint32_t id0 = pci_config_read32(0, s, 0, 0x00);
        if ((id0 & 0xFFFFu) == 0xFFFFu) continue;
        uint32_t hdr = pci_config_read32(0, s, 0, 0x0C);
        int multi = (hdr >> 16) & 0x80u;
        for (int f = 0; f < (multi ? 8 : 1) && !found; f++) {
            uint32_t id = pci_config_read32(0, s, f, 0x00);
            if ((id & 0xFFFFu) == 0xFFFFu) continue;
            uint32_t cls = pci_config_read32(0, s, f, 0x08);
            if ((cls >> 16) == 0x0108u) { /* class 01, subclass 08 */
                n_bus = 0; n_slot = s; n_func = (uint8_t)f;
                found = 1;
            }
        }
    }
    if (!found) {
        serial_puts("[27A] NVMe PCI cihazi yok\n");
        return -1;
    }
    serial_puts("[27A] NVMe @ "); serial_puthex(n_slot);
    serial_puts("."); serial_puthex(n_func); serial_puts("\n");

    /* BAR0 (MMIO) + bus master */
    uint32_t bar_lo = pci_config_read32(n_bus, n_slot, n_func, 0x10);
    uint32_t bar_hi = pci_config_read32(n_bus, n_slot, n_func, 0x14);
    serial_puts("[27A] bar0lo="); serial_puthex(bar_lo);
    serial_puts(" bar0hi="); serial_puthex(bar_hi);
    serial_puts("\n");
    if (bar_lo & 1u) { serial_puts("[27A] BAR0 I/O, garip\n"); return -1; }
    if (((bar_lo >> 1) & 3u) == 2u && bar_hi != 0) {
        /* 64-bit BAR 4GB üstü -> 0xE1100000 penceresine taşı */
        pci_config_write32(n_bus, n_slot, n_func, 0x10, 0xE1100000u | (bar_lo & 0xFu));
        pci_config_write32(n_bus, n_slot, n_func, 0x14, 0);
        bar_lo = pci_config_read32(n_bus, n_slot, n_func, 0x10);
        serial_puts("[27A] BAR tasindi lo="); serial_puthex(bar_lo);
        serial_puts("\n");
    }
    uint32_t base = bar_lo & ~0x3FFFu; /* 16K hizalı */
    if (!base || base >= 0xFEC00000u) {
        serial_puts("[27A] BAR0 kullanilamaz\n");
        return -1;
    }
    /* MEM + bus master aç */
    {
        uint16_t cmd = pci_config_read16(n_bus, n_slot, n_func, 0x04);
        cmd |= 0x06u;
        pci_config_write16(n_bus, n_slot, n_func, 0x04, cmd);
    }
    /* 16K MMIO map'le (UC: PCD|PWT) */
    for (uint32_t a = base; a < base + 0x4000u; a += 0x1000u)
        paging_map(a, a, PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    n_bar = (volatile uint8_t*)(uintptr_t)base;

    /* CAP/VS oku */
    uint64_t cap = r64(R_CAP);
    uint32_t vs = r32(R_VS);
    uint16_t mqes = (uint16_t)(cap & 0xFFFFu);
    n_dstrd = (uint32_t)((cap >> 32) & 0xFu);
    uint32_t mpsmin = (uint32_t)((cap >> 48) & 0xFu);
    (void)mpsmin;
    serial_puts("[27A] bar=0x"); serial_puthex(base);
    serial_puts(" vs="); serial_puthex(vs);
    serial_puts(" mqes="); serial_puthex(mqes);
    serial_puts(" dstrd="); serial_puthex(n_dstrd);
    serial_puts("\n");
    if (!((cap >> 37) & 1u)) { serial_puts("[27A] NVM komut seti yok\n"); return -1; }
    if (mqes < 63u) { serial_puts("[27A] kuyruk girisi yetersiz\n"); return -1; }

    /* Kesmeleri maskele (polling) */
    w32(R_INTMS, 0xFFFFFFFFu);

    /* Kontrolcüyü kapat (önceden açıksa), RDY=0 bekle */
    w32(R_CC, CC_CSS_NVM | CC_MPS_4K | CC_AMS_RR | CC_SHN_NONE | CC_IOSQES_64 | CC_IOCQES_16);
    {
        uint32_t t0 = timer_get_ticks();
        while (r32(R_CSTS) & CSTS_RDY) {
            if ((uint32_t)(timer_get_ticks() - t0) > 500u) {
                serial_puts("[27A] kapatma takildi\n");
                return -1;
            }
        }
    }
    if (r32(R_CSTS) & CSTS_CFS) {
        serial_puts("[27A] fatal durum, sifirlanamadi\n");
        return -1;
    }

    /* Admin kuyrukları: 64 giriş */
    memset(asq, 0, sizeof(asq));
    memset(acq, 0, sizeof(acq));
    a_tail = a_head = 0;
    a_phase = 1;
    w32(R_AQA, ((uint32_t)(ASIZE - 1) << 16) | (ASIZE - 1));
    w64(R_ASQ, phys_of(asq));
    w64(R_ACQ, phys_of(acq));

    /* Aç: EN=1 */
    w32(R_CC, CC_EN | CC_CSS_NVM | CC_MPS_4K | CC_AMS_RR | CC_SHN_NONE | CC_IOSQES_64 | CC_IOCQES_16);    {
        uint32_t t0 = timer_get_ticks();
        while (!(r32(R_CSTS) & CSTS_RDY)) {
            if (r32(R_CSTS) & CSTS_CFS) {
                serial_puts("[27A] acilista fatal\n");
                return -1;
            }
            if ((uint32_t)(timer_get_ticks() - t0) > 500u) {
                serial_puts("[27A] RDY takildi\n");
                return -1;
            }
        }
    }
    serial_puts("[27A] kontrolcu hazir\n");

    /* Identify Controller -> NN */
    if (identify(CNS_CTRL, 0) != 0) {
        serial_puts("[27A] identify ctrl basarisiz\n");
        return -1;
    }
    uint32_t nn;
    memcpy(&nn, ident_buf + 516, 4);
    uint16_t vid;
    memcpy(&vid, ident_buf + 0, 2);
    serial_puts("[27A] vid="); serial_puthex(vid);
    serial_puts(" NN="); serial_puthex(nn);
    serial_puts(" MN=");
    for (int i = 24; i < 24 + 16; i++) {
        char ch = (char)ident_buf[i];
        if (ch >= 32 && ch < 127) serial_putc(ch);
    }
    serial_puts("\n");
    if (nn == 0) { serial_puts("[27A] NN=0, liste yoklanacak\n"); }
    /* NOT: NN bazı emülatörlerde maksimumu verir (QEMU: 256);
     * otoriter olan aktif NS listesidir. */

    /* Aktif NS listesi -> ilk NSID */
    if (identify(CNS_NSLIST, 0) != 0) {
        serial_puts("[27A] NS listesi basarisiz\n");
        return -1;
    }
    memcpy(&nsid0, ident_buf, 4);
    serial_puts("[27A] nsid0="); serial_puthex(nsid0); serial_puts("\n");
    if (nsid0 == 0 || nsid0 == 0xFFFFFFFFu) {
        serial_puts("[27A] aktif NS yok\n");
        return -1;
    }

    /* Identify NS -> LBA biçimi */
    if (identify(CNS_NS, nsid0) != 0) {
        serial_puts("[27A] identify NS basarisiz\n");
        return -1;
    }
    {
        uint64_t nsze;
        uint8_t flbas, nlbaf;
        memcpy(&nsze, ident_buf + 0, 8);
        nlbaf = ident_buf[25];
        flbas = ident_buf[26] & 0x0Fu;
        ns_blocks = nsze;
        if (flbas > nlbaf) { serial_puts("[27A] FLBAS garip\n"); return -1; }
        uint32_t lbaf;
        memcpy(&lbaf, ident_buf + 128 + flbas * 4, 4);
        uint32_t lbads = (lbaf >> 16) & 0xFFu;
        lba_bytes = 1u << lbads;
        serial_puts("[27A] nsze="); serial_puthex((uint32_t)(nsze & 0xFFFFFFFFu));
        serial_puts(" lba="); serial_puthex(lba_bytes);
        serial_puts(" flbas="); serial_puthex(flbas);
        serial_puts("\n");
        if (lba_bytes != 512u && lba_bytes != 4096u) {
            serial_puts("[27A] LBA boyu desteklenmiyor\n");
            return -1;
        }
        lba_shift = (lba_bytes == 4096u) ? 3u : 0u;
    }

    /* Kuyruk sayısı (0-tabanlı) -> en az 1 I/O çifti iste */
    {
        uint32_t nq = 0;
        if (getfeat(F_NUMQ, 0, &nq) != 0) {
            serial_puts("[27A] NUMQ okunamadi\n");
            return -1;
        }
        serial_puts("[27A] maxio="); serial_puthex(nq); serial_puts("\n");
    }
    if (create_ioq() != 0) return -1;
    serial_puts("[27A] I/O kuyrugu hazir\n");

    n_present = 1;
    serial_puts("[27A] NVMe hazir [PASS]\n");
    vga_puts("[27A] NVMe hazir [PASS]\n");
    return 0;
}

uint64_t nvme_capacity_sectors(void) {
    if (!n_present) return 0;
    /* 512B-sektör eşdeğeri */
    return ns_blocks << lba_shift;
}

/* 27B: tek yerel LBA R/W (bounce tampon üzerinden). 0 başarı. */
static int rw_native(uint64_t slba, int is_write) {
    struct sqe s;
    struct cqe c;
    memset(&s, 0, sizeof(s));
    s.cdw0 = ((uint32_t)(is_write ? IO_WRITE : IO_READ) | ((uint32_t)new_cid() << 16));
    s.nsid = nsid0;
    s.prp1 = phys_of(data_buf);
    s.cdw10 = (uint32_t)(slba & 0xFFFFFFFFu);
    s.cdw11 = (uint32_t)(slba >> 32);
    s.cdw12 = 0; /* NLB=1 */
    mb();
    return run_cmd(1, &s, &c);
}

int nvme_read_sector(uint32_t lba512, void* buf) {
    if (!n_present || !buf) return -1;
    if ((uint64_t)lba512 >= nvme_capacity_sectors()) return -1;
    uint32_t flags;
    spin_lock_irqsave(&n_lock, &flags);
    uint64_t slba = (uint64_t)lba512 >> lba_shift;
    int r;
    if (lba_shift == 0) {
        r = rw_native(slba, 0);
        if (r == 0) memcpy(buf, data_buf, 512);
    } else {
        uint32_t idx = lba512 & 7u;
        r = rw_native(slba, 0);
        if (r == 0) memcpy(buf, data_buf + idx * 512u, 512);
    }
    spin_unlock_irqrestore(&n_lock, flags);
    return r;
}

int nvme_write_sector(uint32_t lba512, const void* buf) {
    if (!n_present || !buf) return -1;
    if ((uint64_t)lba512 >= nvme_capacity_sectors()) return -1;
    uint32_t flags;
    spin_lock_irqsave(&n_lock, &flags);
    uint64_t slba = (uint64_t)lba512 >> lba_shift;
    int r;
    if (lba_shift == 0) {
        memcpy(data_buf, buf, 512);
        r = rw_native(slba, 1);
    } else {
        uint32_t idx = lba512 & 7u;
        r = rw_native(slba, 0); /* RMW: önce oku */
        if (r == 0) {
            memcpy(data_buf + idx * 512u, buf, 512);
            r = rw_native(slba, 1);
        }
    }
    spin_unlock_irqrestore(&n_lock, flags);
    return r;
}

int nvme_read_multi(uint32_t lba512, void* buf, uint32_t say512) {
    if (!n_present || !buf) return -1;
    uint8_t* p = (uint8_t*)buf;
    for (uint32_t i = 0; i < say512; i++) {
        if (nvme_read_sector(lba512 + i, p + i * 512u) != 0) return -1;
    }
    return 0;
}

int nvme_write_multi(uint32_t lba512, const void* buf, uint32_t say512) {
    if (!n_present || !buf) return -1;
    const uint8_t* p = (const uint8_t*)buf;
    for (uint32_t i = 0; i < say512; i++) {
        if (nvme_write_sector(lba512 + i, p + i * 512u) != 0) return -1;
    }
    return 0;
}

/* 27B selftest: scratch LBA (son blok) RW karşılaştırma */
int nvme_selftest(void) {
    if (!n_present) {
        serial_puts("[27B] selftest atlandi (cihaz yok)\n");
        return -1;
    }
    static uint8_t wbuf[512];
    static uint8_t rbuf[512];
    uint64_t cap = nvme_capacity_sectors();
    if (cap < 8) {
        serial_puts("[27B] kapasite cok kucuk\n");
        return -1;
    }
    uint32_t lba = (uint32_t)(cap - 1);
    for (int i = 0; i < 512; i++) wbuf[i] = (uint8_t)(0x5A + (i & 0xFF));
    memset(rbuf, 0, sizeof(rbuf));
    if (nvme_write_sector(lba, wbuf) != 0) {
        serial_puts("[27B] selftest WRITE [FAIL]\n");
        vga_puts("[27B] selftest WRITE [FAIL]\n");
        return -1;
    }
    if (nvme_read_sector(lba, rbuf) != 0) {
        serial_puts("[27B] selftest READ [FAIL]\n");
        return -1;
    }
    if (memcmp(wbuf, rbuf, 512) != 0) {
        serial_puts("[27B] selftest COMPARE [FAIL]\n");
        vga_puts("[27B] selftest COMPARE [FAIL]\n");
        return -1;
    }
    serial_puts("[27B] NVMe sector RW [PASS]\n");
    vga_puts("[27B] NVMe sector RW [PASS]\n");
    return 0;
}
