#include <drivers/gpt.h>
#include <drivers/nvme.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 27C: GPT tarama. UEFI/GPT: LBA1 header, girişler varsayılan LBA2. */

const uint8_t CHAROS_PART_GUID[16] = {
    0xD4, 0xC3, 0xB2, 0xA1, 0xF6, 0xE5, 0x7B, 0x4A,
    0x8C, 0x9D, 0xE0, 0xF1, 0xA2, 0xB3, 0xC4, 0xD5
};

static uint32_t crc_tab[256];
static int crc_ready = 0;

static void crc_init(void) {
    if (crc_ready) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc_tab[i] = c;
    }
    crc_ready = 1;
}

static uint32_t crc32(const uint8_t* p, uint32_t len) {
    crc_init();
    uint32_t c = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++)
        c = crc_tab[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static uint64_t rd64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}
static uint32_t rd32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int guid_zero(const uint8_t* g) {
    for (int i = 0; i < 16; i++) if (g[i]) return 0;
    return 1;
}

/* Önbellek: 27D aynı taramayı tekrar okumaz */
static struct gpt_part cache_parts[GPT_MAX_PARTS];
static int cache_n = -1; /* -1 = taranmadı */

int gpt_scan(int (*read512)(uint32_t lba, void* buf),
             struct gpt_part* parts, int maxparts) {
    static uint8_t sec[512];
    if (!read512 || !parts || maxparts <= 0) return -1;

    /* LBA1: GPT header */
    if (read512(1, sec) != 0) return -1;
    if (memcmp(sec, "EFI PART", 8) != 0) return -1;
    uint32_t hdr_size = rd32(sec + 12);
    if (hdr_size < 92 || hdr_size > 512) return -1;
    uint32_t hdr_crc, calc;
    {
        uint8_t tmp[512];
        memcpy(tmp, sec, hdr_size);
        memcpy(&hdr_crc, tmp + 16, 4);
        memset(tmp + 16, 0, 4);
        calc = crc32(tmp, hdr_size);
        if (calc != hdr_crc) {
            serial_puts("[27C] header CRC bozuk\n");
            return -1;
        }
    }
    uint64_t ent_lba = rd64(sec + 72);
    uint32_t num_ent = rd32(sec + 80);
    uint32_t ent_size = rd32(sec + 84);
    uint32_t ent_crc;
    memcpy(&ent_crc, sec + 88, 4);
    if (ent_size != 128) {
        serial_puts("[27C] entry boyu desteklenmiyor\n");
        return -1;
    }
    if (num_ent == 0 || num_ent > 256) return -1;
    if (ent_lba > 0xFFFFFFFFu) return -1;

    /* Girişleri oku (128B x N, LBA başına 4 giriş) + CRC */
    uint32_t total_bytes = num_ent * ent_size;
    uint32_t total_sec = (total_bytes + 511) / 512;
    /* CRC'yi akan hesapla, girişleri parça parça işle */
    crc_init();
    uint32_t run = 0xFFFFFFFFu;
    int n = 0;
    for (uint32_t s = 0; s < total_sec; s++) {
        if (read512((uint32_t)ent_lba + s, sec) != 0) return -1;
        for (uint32_t i = 0; i < 512; i++)
            run = crc_tab[(run ^ sec[i]) & 0xFFu] ^ (run >> 8);
        /* Bu sektördeki girişler (son sektör eksik olabilir) */
        for (int e = 0; e < 4; e++) {
            uint32_t idx = s * 4 + e;
            if (idx >= num_ent) break;
            uint8_t* en = sec + e * 128;
            if (guid_zero(en)) continue; /* boş giriş */
            if (n < maxparts && n < GPT_MAX_PARTS) {
                struct gpt_part* p = &parts[n];
                memcpy(p->type_guid, en, 16);
                memcpy(p->uniq_guid, en + 16, 16);
                p->start_lba = rd64(en + 32);
                p->end_lba = rd64(en + 40);
                for (int c = 0; c < 36; c++) {
                    uint8_t lo = en[56 + c * 2];
                    p->name[c] = (lo >= 32 && lo < 127) ? (char)lo : '\0';
                    if (!p->name[c]) { /* kalanı kes */
                        for (int k = c; k < 37; k++) p->name[k] = '\0';
                        break;
                    }
                }
                p->name[36] = '\0';
                n++;
            }
        }
    }
    run ^= 0xFFFFFFFFu;
    if (run != ent_crc) {
        serial_puts("[27C] entry CRC bozuk\n");
        return -1;
    }
    return n;
}

int gpt_find_charos(const struct gpt_part* parts, int n) {
    if (!parts) return -1;
    for (int i = 0; i < n; i++) {
        if (memcmp(parts[i].type_guid, CHAROS_PART_GUID, 16) == 0) return i;
    }
    return -1;
}

int gpt_find_empty(const struct gpt_part* parts, int n, uint64_t min_sectors) {
    (void)parts; (void)n; (void)min_sectors;
    /* Boş girişler gpt_scan'de atlanır; ham tarama gerekir. Şimdilik yok. */
    return -1;
}

/* NVMe üstünde tara + önbelleğe al. Dönüş: bölüm sayısı / -1 */
int gpt_scan_nvme(void) {
    if (!nvme_present()) return -1;
    if (cache_n >= 0) return cache_n;
    int n = gpt_scan(nvme_read_sector, cache_parts, GPT_MAX_PARTS);
    cache_n = (n < 0) ? -2 : n; /* -2 = yok/bozuk (tekrar okuma) */
    return (n < 0) ? -1 : n;
}

void gpt_invalidate(void) { cache_n = -1; }

const struct gpt_part* gpt_cached(int* n) {
    if (cache_n < 0) return 0;
    if (n) *n = cache_n;
    return cache_parts;
}

static void print_guid(const uint8_t* g) {
    /* karışık-endian GUID'i klasik dizge gibi bas */
    uint32_t d1; uint16_t d2, d3;
    memcpy(&d1, g, 4); memcpy(&d2, g + 4, 2); memcpy(&d3, g + 6, 2);
    serial_puthex(d1); serial_puts("-");
    serial_puthex(d2); serial_puts("-");
    serial_puthex(d3); serial_puts("-");
    for (int i = 8; i < 16; i++) {
        const char* hx = "0123456789ABCDEF";
        char b[3] = { hx[g[i] >> 4], hx[g[i] & 15], 0 };
        serial_puts(b);
    }
}

/* 27C selftest: NVMe'de GPT tara, bölümleri listele */
int gpt_selftest(void) {
    if (!nvme_present()) {
        serial_puts("[27C] selftest atlandi (NVMe yok)\n");
        return -1;
    }
    int n = gpt_scan_nvme();
    if (n < 0) {
        serial_puts("[27C] GPT yok/bozuk [FAIL]\n");
        vga_puts("[27C] GPT yok/bozuk [FAIL]\n");
        return -1;
    }
    serial_puts("[27C] GPT bolumler n="); serial_puthex(n); serial_puts("\n");
    for (int i = 0; i < n; i++) {
        serial_puts("  #"); serial_puthex(i);
        serial_puts(" "); serial_puts(cache_parts[i].name);
        serial_puts(" LBA "); serial_puthex((uint32_t)cache_parts[i].start_lba);
        serial_puts(".."); serial_puthex((uint32_t)cache_parts[i].end_lba);
        serial_puts(" tip ");
        print_guid(cache_parts[i].type_guid);
        serial_puts("\n");
    }
    int ci = gpt_find_charos(cache_parts, n);
    if (ci < 0) {
        serial_puts("[27C] charOS bolumu yok [FAIL]\n");
        return -1;
    }
    serial_puts("[27C] GPT scan [PASS]\n");
    vga_puts("[27C] GPT scan [PASS]\n");
    return 0;
}
