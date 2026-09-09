#include <memory/pmm.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>
#include <stdint.h>

/* 26A: Multiboot2 (UEFI) info'yu Multiboot1 (BIOS) formatına çevirir.
 *
 * UEFI'de GRUB multiboot2 protokolüyle 32-bit kernel yükler ve uzun uzun
 * tag listesi verir (total_size + {type,size,data} tag'leri). Tüketiciler
 * (pmm_init, fb_init) bildikleri mb1 düzenini okur; çeviri sonucu sabit bir
 * mb1 tamponuna yazılır ve kernel_main o adresi kullanır. */

#define MB2_BOOT_MAGIC 0x36D76289  /* eax'te dönen magic */
#define MB1_BOOT_MAGIC 0x2BADB002

#define MBI_MMAP_MAX 48

/* Çevrilmiş mb1 bilgisi (ilk 128B, mb1 düzeni: flags..+/fb_type) */
static uint8_t mb1_info[128] __attribute__((aligned(16)));
static struct multiboot_mmap_entry mb1_entries[MBI_MMAP_MAX];

struct mb2_tag_hdr {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

/* mb1 info üzerinde fb alanları (fb.c'deki mb_info ile aynı offset'ler) */
#define OFF_FB_ADDR   88
#define OFF_FB_PITCH  96
#define OFF_FB_WIDTH  100
#define OFF_FB_HEIGHT 104
#define OFF_FB_BPP    108
#define OFF_FB_TYPE   109

static uint32_t rd32(void* p) { return *(uint32_t*)p; }
static uint64_t rd64(void* p) { return *(uint64_t*)p; }

/* 26A: UEFI ACPI RSDP kopyası (mb2 tag 14/15 yükü).
 * Loader RAM'i yüksek adreste olabilir; baytlar kernel statiğine alınır
 * (paging sonrası da okunabilir). Tabloların kendisi firmware RAM'inde
 * durur; içindeki adresler acpi_map_range ile map'lenir. */
static uint8_t g_rsdp_bytes[36];
static uint32_t g_rsdp_len = 0;

const void* mb2_rsdp(void) { return g_rsdp_len ? (const void*)g_rsdp_bytes : 0; }
uint32_t mb2_rsdp_len(void) { return g_rsdp_len; }
uint32_t mb2_translate(uint32_t mb2_addr) {
    if (!mb2_addr) return 0;
    uint32_t total_size = rd32((void*)mb2_addr);
    if (total_size < 16 || total_size > 0x10000) {
        if (serial_is_ready()) {
            serial_puts("[26A] mb2 total_size anormal: "); serial_puthex(total_size);
            serial_puts("\n");
        }
        return 0;
    }

    memset(mb1_info, 0, sizeof(mb1_info));
    memset(mb1_entries, 0, sizeof(mb1_entries));

    uint32_t flags = (1u << 0); /* align flag kabul et; kesinlik meminfo'dan */
    uint32_t mem_lower = 640, mem_upper = 0;
    uint32_t mmap_len = 0, mmap_cnt = 0;

    int has_meminfo = 0, has_mmap = 0, has_fb = 0;

    uint32_t off = 8; /* total_size + reserved */
    while (off + 8 <= total_size) {
        struct mb2_tag_hdr* tag = (struct mb2_tag_hdr*)(mb2_addr + off);
        uint32_t t = tag->type;
        uint32_t sz = tag->size;
        void* data = (void*)((uint8_t*)tag + 8);
        if (sz < 8) break;
        if (t == 0) break; /* end tag */

        switch (t) {
        case 4: { /* basic meminfo: mem_lower, mem_upper (KiB) */
            if (sz >= 16) {
                mem_lower = rd32(data);
                mem_upper = rd32((uint8_t*)data + 4);
                has_meminfo = 1;
            }
            break;
        }
        case 6: { /* memory map */
            if (sz >= 16) {
                uint32_t entry_size = rd32(data);
                uint32_t n = (sz - 8) / entry_size;
                if (n > MBI_MMAP_MAX) n = MBI_MMAP_MAX;
                uint8_t* p = (uint8_t*)data + 8;
                for (uint32_t i = 0; i < n; i++) {
                    mb1_entries[mmap_cnt].size = 20;
                    mb1_entries[mmap_cnt].base_addr = rd64(p);
                    mb1_entries[mmap_cnt].length = rd64(p + 8);
                    mb1_entries[mmap_cnt].type = rd32(p + 16);
                    mmap_cnt++;
                    if (mmap_cnt >= MBI_MMAP_MAX) break;
                    p += entry_size;
                }
                mmap_len = mmap_cnt * 24;
                has_mmap = (mmap_cnt > 0);
            }
            break;
        }
        case 8: { /* framebuffer */
            if (sz >= 24) {
                uint64_t fba = rd64(data);
                uint32_t pitch = rd32((uint8_t*)data + 8);
                uint32_t width = rd32((uint8_t*)data + 12);
                uint32_t height = rd32((uint8_t*)data + 16);
                uint8_t bpp = *(uint8_t*)((uint8_t*)data + 20);
                uint8_t ftype = *(uint8_t*)((uint8_t*)data + 21);
                uint32_t fba32 = (uint32_t)fba; /* >4GiB fb 32-bit adreslenemez */
                if (fba32 && pitch && width && height && bpp >= 8 && bpp <= 32) {
                    *(uint32_t*)(mb1_info + OFF_FB_ADDR) = fba32;
                    *(uint32_t*)(mb1_info + OFF_FB_ADDR + 4) = 0; /* üst 32b */
                    *(uint32_t*)(mb1_info + OFF_FB_PITCH) = pitch;
                    *(uint32_t*)(mb1_info + OFF_FB_WIDTH) = width;
                    *(uint32_t*)(mb1_info + OFF_FB_HEIGHT) = height;
                    mb1_info[OFF_FB_BPP] = bpp;
                    mb1_info[OFF_FB_TYPE] = ftype;
                    has_fb = 1;
                }
            }
            break;
        }
        default:
            break;
        }
        if (t == 14 && sz >= 28) {
            /* ACPI RSDP v1 kopyası (20B) -> kernel statiğine */
            const uint8_t* src = (const uint8_t*)tag + 8;
            for (int i = 0; i < 20; i++) g_rsdp_bytes[i] = src[i];
            if (g_rsdp_len < 20) g_rsdp_len = 20;
        } else if (t == 15 && sz >= 44) {
            /* ACPI XSDP kopyası (36B) — v2 tercih edilir */
            const uint8_t* src = (const uint8_t*)tag + 8;
            for (int i = 0; i < 36; i++) g_rsdp_bytes[i] = src[i];
            g_rsdp_len = 36;
        }
        off += (sz + 7) & ~7u; /* 8B hizalı */
        if (off <= 8) break;
    }

    if (has_meminfo) flags |= (1u << 1); /* mem_lower/mem_upper geçerli */
    if (has_mmap) {
        flags |= (1u << 6);              /* mmap geçerli */
        *(uint32_t*)(mb1_info + 44) = mmap_len;
        *(uint32_t*)(mb1_info + 48) = (uint32_t)(uintptr_t)mb1_entries;
    }
    if (has_fb) flags |= (1u << 12);     /* framebuffer geçerli */

    *(uint32_t*)(mb1_info + 0) = flags;
    *(uint32_t*)(mb1_info + 4) = mem_lower;
    *(uint32_t*)(mb1_info + 8) = mem_upper;

    if (serial_is_ready()) {
        serial_puts("[26A] mb2->mb1 ok flags="); serial_puthex(flags);
        serial_puts(" mem_upper="); serial_puthex(mem_upper);
        serial_puts(" mmap="); serial_puthex(mmap_cnt);
        serial_puts(" fb="); serial_puthex(has_fb);
        serial_puts("\n");
    }

    return (uint32_t)(uintptr_t)mb1_info;
}

/* magic'e göre kullanılacak info adresini verir (mb1 ise aynen, mb2 ise çevir) */
uint32_t boot_info(uint32_t magic, uint32_t mboot_ptr) {
    if (magic == MB1_BOOT_MAGIC) return mboot_ptr;
    if (magic == MB2_BOOT_MAGIC) {
        uint32_t p = mb2_translate(mboot_ptr);
        return p ? p : 0;
    }
    return 0;
}