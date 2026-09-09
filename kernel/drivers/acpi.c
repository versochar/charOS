#include <drivers/acpi.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14G: ACPI tablo yürüyüşü + HPET. BIOS altı bellek (0xF0000) identity
 * map'lidir; üst RAM'deki tablolar dokunulmadan önce map edilir. */

/* "d" kısıtı: sabit + değişken portların ikisi de derlenir */
static inline void a_outb(uint16_t p, uint8_t v) {
    asm volatile("outb %0,%1"::"a"(v),"d"(p));
}
static inline void a_outw(uint16_t p, uint16_t v) {
    asm volatile("outw %0,%1"::"a"(v),"d"(p));
}
static inline uint16_t a_inw(uint16_t p) {
    uint16_t r; asm volatile("inw %1,%0":"=a"(r):"d"(p)); return r;
}

struct __attribute__((packed)) sdt_hdr {
    char sig[4];
    uint32_t len;
    uint8_t rev;
    uint8_t csum;
    char oem[6];
    char oemtable[8];
    uint32_t oemrev;
    uint32_t creator;
    uint32_t creatorrev;
};

struct __attribute__((packed)) gas {
    uint8_t space;   /* 0=mem, 1=io */
    uint8_t width;
    uint8_t offset;
    uint8_t access;
    uint64_t addr;
};

/* FADT: yalnızca ihtiyaç duyulan alanlar (ofsetler ACPI spec) */
struct __attribute__((packed)) fadt_body {
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved1;
    uint8_t pm_prof;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    /* ... devamı lazım değil ... */
};

static int acpi_on = 0;
static struct sdt_hdr* facp = 0;
static struct sdt_hdr* hpet_tbl = 0;
static uint32_t pm1a_cnt = 0;
static uint8_t hpet_mapped = 0;
static uint32_t hpet_base = 0;

static int sum_ok(const uint8_t* p, uint32_t len) {
    uint8_t s = 0;
    for (uint32_t i = 0; i < len; i++) s += p[i];
    return s == 0;
}

static void hpet_enable(void);

/* Fiziksel aralığı identity map'le + PMM'den düş (çift tahsis yok).
 * ACPI tabloları üst RAM'dedir; dokunmadan önce çağrılmalıdır.
 * 26A: UEFI'de tablolar 256MB üstünde de olabilir; map 4GB'a kadar açık,
 * PMM rezervi pmm_reserve'in kendi sınırına emanet. */
static void acpi_map_range(uint32_t phys, uint32_t len) {
    if (!phys || !len) return;
    uint32_t start = phys & ~0xFFFu;
    uint32_t end = (phys + len + 0xFFFu) & ~0xFFFu;
    if (end <= start) return;
    for (uint32_t a = start; a < end; a += 0x1000u) {
        paging_map(a, a, PAGE_PRESENT | PAGE_RW);
        pmm_reserve(a, 0x1000u);
    }
}

int acpi_init(void) {
    if (acpi_on) return 0;
    /* 26A: UEFI'den gelen RSDP kopyası (mb2 tag 14/15) öncelikli;
     * BIOS'ta yok -> klasik ROM taraması. */
    const uint8_t* rsdp = 0;
    {
        extern const void* mb2_rsdp(void);
        extern uint32_t mb2_rsdp_len(void);
        const uint8_t* c = (const uint8_t*)mb2_rsdp();
        uint32_t cl = mb2_rsdp_len();
        if (c && cl >= 20 && c[0]=='R' && c[1]=='S' && c[2]=='D' && c[3]==' ' &&
            c[4]=='P' && c[5]=='T' && c[6]=='R' && c[7]==' ' && sum_ok(c, 20)) {
            uint8_t rev = c[15];
            if (rev >= 2 && cl >= 36) {
                /* XSDP genişletilmiş sağlama */
                uint8_t s = 0;
                for (uint32_t i = 0; i < 36; i++) s += c[i];
                if (s == 0) { rsdp = c; serial_puts("[14G] XSDP (mb2)\n"); }
            }
            if (!rsdp) { rsdp = c; serial_puts("[14G] RSDP (mb2)\n"); }
        }
    }
    if (!rsdp) {
    /* RSDP tara: 0xE0000-0xFFFFF, 16 bayt adım */
    for (uint32_t a = 0xE0000; a < 0x100000; a += 16) {
        const uint8_t* p = (const uint8_t*)a;
        if (p[0]=='R' && p[1]=='S' && p[2]=='D' && p[3]==' ' &&
            p[4]=='P' && p[5]=='T' && p[6]=='R' && p[7]==' ') {
            if (sum_ok(p, 20)) { rsdp = p; break; }
        }
    }
    }
    if (!rsdp) {
        serial_puts("[14G] RSDP yok\n");
        return -1;
    }
    /* RSDT + tablolar üst RAM'de olabilir (map'siz) — önce map'le.
     * Map'siz okuma #PF verip boot task'ını öldürür (14G bulgusu). */
    uint32_t rsdt = *(const uint32_t*)(rsdp + 16);
    if (!rsdt) return -1;
    acpi_map_range(rsdt, 36);
    struct sdt_hdr* rh = (struct sdt_hdr*)rsdt;
    if (rh->sig[0]!='R' || rh->sig[1]!='S' || rh->sig[2]!='D' || rh->sig[3]!='T')
        return -1;
    if (rh->len < 36 || rh->len > 4096) return -1;
    acpi_map_range(rsdt, rh->len);
    if (!sum_ok((const uint8_t*)rh, rh->len)) return -1;
    uint32_t n = (rh->len - 36) / 4;
    const uint32_t* ent = (const uint32_t*)((const uint8_t*)rh + 36);
    for (uint32_t i = 0; i < n && i < 64; i++) {
        uint32_t ea = ent[i];
        if (!ea) continue;
        acpi_map_range(ea, 36); /* başlık */
        struct sdt_hdr* h = (struct sdt_hdr*)ea;
        if (h->len < 36 || h->len > 65536) continue;
        acpi_map_range(ea, h->len); /* tamamı */
        if (!sum_ok((const uint8_t*)h, h->len)) continue;
        if (h->sig[0]=='F' && h->sig[1]=='A' && h->sig[2]=='C' && h->sig[3]=='P')
            facp = h;
        else if (h->sig[0]=='H' && h->sig[1]=='P' && h->sig[2]=='E' && h->sig[3]=='T')
            hpet_tbl = h;
    }
    if (!facp) {
        serial_puts("[14G] FACP yok\n");
        return -1;
    }
    {
        struct fadt_body* f = (struct fadt_body*)((uint8_t*)facp + 36);
        pm1a_cnt = f->pm1a_cnt_blk & 0xFFFF;
    }
    /* HPET tablosu + MMIO map (identity, uncached) */
    if (hpet_tbl && hpet_tbl->len >= 44) {
        struct gas* g = (struct gas*)((uint8_t*)hpet_tbl + 40);
        if (g->space == 0 && g->addr) {
            uint32_t base = (uint32_t)(g->addr & ~0xFFFu);
            paging_map(base, base, PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
            pmm_reserve(base, 0x1000);
            hpet_base = base;
            hpet_mapped = 1;
            hpet_enable();
        }
    }
    acpi_on = 1;
    serial_puts("[14G] ACPI hazır (FACP");
    serial_puts(hpet_tbl ? "+HPET" : "");
    serial_puts(")\n");
    return 0;
}

int acpi_have_fadt(void) { return acpi_on && facp != 0; }
int acpi_have_hpet(void) { return acpi_on && hpet_tbl != 0 && hpet_mapped; }
uint32_t acpi_pm1a_cnt(void) { return pm1a_cnt; }
uint16_t acpi_slp_typa(void) { return (uint16_t)(0x7u << 10); } /* QEMU piix S5 */

void acpi_enable(void) {
    if (!acpi_on || !facp) return;
    struct fadt_body* f = (struct fadt_body*)((uint8_t*)facp + 36);
    if (!f->smi_cmd || !f->acpi_enable) return;
    a_outb((uint16_t)f->smi_cmd, f->acpi_enable);
    for (volatile int i = 0; i < 3000000; i++) {
        /* SCI_EN (PM1a CNT bit0) gelene kadar bekle */
        if (pm1a_cnt) {
            if (a_inw((uint16_t)pm1a_cnt) & 1) break;
        } else break;
        if ((i & 0xFFF) == 0) asm volatile("pause");
    }
}

void acpi_poweroff(void) {
    /* DÖNMEZ. S5: SLP_TYP + SLP_EN (bit13). Önce test YOK. */
    if (pm1a_cnt) {
        a_outw((uint16_t)pm1a_cnt, (uint16_t)(acpi_slp_typa() | (1u << 13)));
    }
    /* Yedek: klavye reset + triple fault */
    a_outb(0x64, 0xFE);
    asm volatile("cli");
    for (;;) asm volatile("hlt");
}

void acpi_reboot(void) {
    /* FADT reset register (genelde 0xCF9, değer 0x06) */
    if (acpi_on && facp && facp->rev >= 2 && facp->len >= 128) {
        struct gas* g = (struct gas*)((uint8_t*)facp + 116);
        if (g->space == 1 && g->addr) {
            uint8_t rv = *((uint8_t*)((uint8_t*)facp + 128));
            if (!rv) rv = 0x06;
            a_outb((uint16_t)g->addr, rv);
        }
    }
    a_outb(0x64, 0xFE);
    asm volatile("cli");
    for (;;) asm volatile("hlt");
}

/* 28C: S3 uyku (basit skeleton) */
void acpi_s3_sleep(void) {
    serial_puts("[28C] ACPI S3 uyku isteği (skeleton)\n");
}

/* HPET registerları (taban + ofset) */
#define HPET_CAP   0x00
#define HPET_CONF  0x10
#define HPET_COUNT 0xF0

/* 14G: ana sayacı aç (firmware kapalı bırakabilir; ENABLE_CNF bit0) */
static void hpet_enable(void) {
    if (!hpet_mapped) return;
    volatile uint32_t* cfg = (volatile uint32_t*)(hpet_base + HPET_CONF);
    cfg[0] = cfg[0] | 1u;
}

static uint32_t hpet_read32(uint32_t off) {
    volatile uint32_t* r = (volatile uint32_t*)(hpet_base + off);
    return *r;
}

int hpet_present(void) { return acpi_on && hpet_mapped; }

uint32_t hpet_period_fs(void) {
    if (!hpet_present()) return 0;
    return hpet_read32(HPET_CAP + 4); /* üst 32 bit: femtosaniye */
}

uint64_t hpet_read(void) {
    if (!hpet_present()) return 0;
    /* 64-bit tutarlı okuma: hi-lo-hi */
    uint32_t hi1, lo, hi2;
    do {
        hi1 = hpet_read32(HPET_COUNT + 4);
        lo = hpet_read32(HPET_COUNT);
        hi2 = hpet_read32(HPET_COUNT + 4);
    } while (hi1 != hi2);
    return ((uint64_t)hi1 << 32) | lo;
}

int acpi_selftest(void) {
    int ok = 1;
    if (!acpi_on && acpi_init() != 0) return -1;
    if (!acpi_have_fadt()) ok = 0;
    if (!acpi_have_hpet()) ok = 0;
    if (acpi_pm1a_cnt() == 0) ok = 0;
    /* HPET sayacı ilerlemeli */
    if (ok) {
        uint64_t a = hpet_read();
        uint64_t b = a;
        for (int i = 0; i < 1000 && b == a; i++) {
            b = hpet_read();
            if ((i & 0xFF) == 0) asm volatile("pause");
        }
        if (b == a) ok = 0;
        if (hpet_period_fs() == 0) ok = 0;
    }
    if (ok) {
        serial_puts("[14G] acpi/hpet [PASS]\n");
        vga_puts("[14G] acpi/hpet [PASS]\n");
        return 0;
    }
    serial_puts("[14G] acpi [FAIL]\n");
    vga_puts("[14G] acpi [FAIL]\n");
    return -1;
}
