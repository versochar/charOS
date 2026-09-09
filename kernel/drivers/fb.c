#include <drivers/fb.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18B: VESA linear framebuffer.
 * GRUB (multiboot header video isteği) modu kurar; kernel LFB'yi 1:1 map'ler,
 * PMM'den ayırdığı RAM'i arka buffer yapar, flip = memcpy. */

#define FB_BACK_VIRT 0xE1000000u
#define FB_MAX_BYTES (16u * 1024u * 1024u) /* QEMU stdvga VRAM üst sınırı */

struct mb_info {
    uint32_t flags;       /* +0 */
    uint32_t mem_lower;   /* +4 */
    uint32_t mem_upper;   /* +8 */
    uint8_t pad[76];      /* +12..87 (boot_device..vbe_interface_len) */
    uint64_t fb_addr;     /* +88 */
    uint32_t fb_pitch;    /* +96 */
    uint32_t fb_width;    /* +100 */
    uint32_t fb_height;   /* +104 */
    uint8_t fb_bpp;       /* +108 */
    uint8_t fb_type;      /* +109 */
} __attribute__((packed));

static int fb_on = 0;
static uint32_t fb_w = 0, fb_h = 0, fb_bpp = 0, fb_pitch = 0;
static uint8_t* fb_front = 0;
static uint8_t* fb_back = 0;
static uint32_t fb_bytes = 0;

/* Kirli-dikdörtgen flip: ön tampon uncached VRAM olduğundan (bkz. fb_init)
 * tam 3MB kopya her karede takılma yapar. Yalnız değişen bölge taşınır. */
static int fb_ddirty = 0;
static uint32_t fb_dx0 = 0, fb_dy0 = 0, fb_dx1 = 0, fb_dy1 = 0;

void fb_damage(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) {
    if (!fb_on) return;
    if (x0 >= fb_w || y0 >= fb_h) return;
    if (x1 > fb_w) x1 = fb_w;
    if (y1 > fb_h) y1 = fb_h;
    if (x0 >= x1 || y0 >= y1) return;
    if (!fb_ddirty) {
        fb_dx0 = x0; fb_dy0 = y0; fb_dx1 = x1; fb_dy1 = y1;
        fb_ddirty = 1;
        return;
    }
    if (x0 < fb_dx0) fb_dx0 = x0;
    if (y0 < fb_dy0) fb_dy0 = y0;
    if (x1 > fb_dx1) fb_dx1 = x1;
    if (y1 > fb_dy1) fb_dy1 = y1;
}

void fb_damage_all(void) {
    if (!fb_on) return;
    fb_dx0 = 0; fb_dy0 = 0; fb_dx1 = fb_w; fb_dy1 = fb_h;
    fb_ddirty = 1;
}

static inline uint8_t fb_inb(uint16_t p) {
    uint8_t r; asm volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;
}

static uint32_t fb_bypp(void) { return (fb_bpp + 7) / 8; }

int fb_active(void) { return fb_on; }

uint8_t* fb_back_buffer(void) { return fb_back; }
uint32_t fb_pitch_bytes(void) { return fb_pitch; }
uint8_t fb_bytes_per_pixel(void) { return (uint8_t)fb_bypp(); }

void fb_get_mode(uint32_t* w, uint32_t* h, uint32_t* bpp) {
    if (w) *w = fb_w;
    if (h) *h = fb_h;
    if (bpp) *bpp = fb_bpp;
}

int fb_init(uint32_t mb_info_addr) {
    struct mb_info* mbi = (struct mb_info*)mb_info_addr;
    if (!mb_info_addr || !(mbi->flags & (1u << 12))) {
        serial_puts("[18B] multiboot fb bilgisi yok, text modda devam\n");
        vga_puts("[18B] no framebuffer info, text mode\n");
        return -1;
    }
    uint32_t phys = (uint32_t)mbi->fb_addr; /* 32-bit üstü VRAM yok say */
    uint32_t w = mbi->fb_width, h = mbi->fb_height;
    uint32_t bpp = mbi->fb_bpp, pitch = mbi->fb_pitch;
    if (!phys || !w || !h || !pitch || bpp < 8 || bpp > 32) {
        serial_puts("[18B] fb geometrisi geçersiz\n");
        return -1;
    }
    if (w > 2048 || h > 2048) {
        serial_puts("[18B] fb çok büyük, atlandı\n");
        return -1;
    }
    uint32_t size = pitch * h;
    if (size == 0 || size > FB_MAX_BYTES) {
        serial_puts("[18B] fb boyutu aralık dışı\n");
        return -1;
    }
    /* LFB'yi 1:1 map'le (uncached, LAPIC ile aynı politika) */
    uint32_t pages = (size + 0xFFFu) / 0x1000u;
    for (uint32_t i = 0; i < pages; i++)
        paging_map(phys + i * 0x1000u, phys + i * 0x1000u,
                   PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    /* Arka buffer: PMM frame'leri + sabit sanal taban */
    uint32_t bpages = (size + 0xFFFu) / 0x1000u;
    for (uint32_t i = 0; i < bpages; i++) {
        uint32_t f = pmm_alloc_frame();
        if (!f) {
            serial_puts("[18B] arka buffer OOM\n");
            return -1;
        }
        paging_map(FB_BACK_VIRT + i * 0x1000u, f, PAGE_PRESENT | PAGE_RW);
    }
    fb_w = w; fb_h = h; fb_bpp = bpp; fb_pitch = pitch;
    fb_bytes = size;
    fb_front = (uint8_t*)phys;
    fb_back = (uint8_t*)FB_BACK_VIRT;
    fb_on = 1;
    memset(fb_back, 0, size);
    serial_puts("[18B] fb "); serial_puthex(w);
    serial_puts("x"); serial_puthex(h);
    serial_puts("x"); serial_puthex(bpp);
    serial_puts(" lfb=0x"); serial_puthex(phys);
    serial_puts(" pitch="); serial_puthex(pitch);
    serial_puts("\n");
    vga_puts("[18B] framebuffer active\n");
    return 0;
}

/* Rengi bpp'ye göre piksel baytlarına yaz */
static void px_write(uint8_t* p, uint32_t color) {
    uint32_t bypp = fb_bypp();
    if (fb_bpp == 32 || (fb_bpp > 24)) {
        p[0] = color & 0xFF; p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF; p[3] = (color >> 24) & 0xFF;
    } else if (fb_bpp > 16) {
        p[0] = color & 0xFF; p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    } else if (fb_bpp > 8) {
        uint16_t v = (uint16_t)color;
        p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
    } else {
        p[0] = (uint8_t)color;
    }
    (void)bypp;
}

static uint32_t px_read(const uint8_t* p) {
    if (fb_bpp > 24) return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
    if (fb_bpp > 16) return p[0] | (p[1] << 8) | (p[2] << 16);
    if (fb_bpp > 8) return p[0] | (p[1] << 8);
    return p[0];
}

void fb_clear(uint32_t color) {
    if (!fb_on) return;
    uint32_t bypp = fb_bypp();
    /* Hızlı yol: satır desenini kur, satır satır kopyala */
    uint8_t row[2048 * 4];
    if (fb_w * bypp > sizeof(row)) return;
    for (uint32_t x = 0; x < fb_w; x++) px_write(row + x * bypp, color);
    for (uint32_t y = 0; y < fb_h; y++)
        memcpy(fb_back + y * fb_pitch, row, fb_w * bypp);
    fb_damage(0, 0, fb_w, fb_h);
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_on || x >= fb_w || y >= fb_h) return;
    px_write(fb_back + y * fb_pitch + x * fb_bypp(), color);
    fb_damage(x, y, x + 1, y + 1);
}

uint32_t fb_getpixel_front(uint32_t x, uint32_t y) {
    if (!fb_on || x >= fb_w || y >= fb_h) return 0;
    return px_read(fb_front + y * fb_pitch + x * fb_bypp());
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!fb_on) return;
    if (x >= fb_w || y >= fb_h) return;
    if (x + w > fb_w) w = fb_w - x;
    if (y + h > fb_h) h = fb_h - y;
    uint32_t bypp = fb_bypp();
    for (uint32_t yy = y; yy < y + h; yy++)
        for (uint32_t xx = x; xx < x + w; xx++)
            px_write(fb_back + yy * fb_pitch + xx * bypp, color);
    fb_damage(x, y, x + w, y + h);
}

void fb_flip(void) {
    if (!fb_on || !fb_ddirty) return;
    uint32_t bypp = fb_bypp();
    uint32_t roww = (fb_dx1 - fb_dx0) * bypp;
    for (uint32_t y = fb_dy0; y < fb_dy1; y++)
        memcpy(fb_front + y * fb_pitch + fb_dx0 * bypp,
               fb_back + y * fb_pitch + fb_dx0 * bypp, roww);
    fb_ddirty = 0;
}

int fb_vblank_wait(void) {
    /* VGA status port bit3: 1 = retrace içinde.
     * Modern UEFI ve PCIe kartlarda 0x3DA portu 0xFF döner (floating bus).
     * Bu durumda beklemeden hemen çık. */
    uint8_t status = fb_inb(0x3DA);
    if (status == 0xFF) return 0;

    for (volatile int i = 0; i < 50000; i++) {
        if (!(fb_inb(0x3DA) & 0x08)) break;
        if ((i & 0xFF) == 0) asm volatile("pause");
    }
    for (volatile int i = 0; i < 50000; i++) {
        if (fb_inb(0x3DA) & 0x08) return 1;
        if ((i & 0xFF) == 0) asm volatile("pause");
    }
    return 0;
}

int fb_selftest(void) {
    if (!fb_on) {
        serial_puts("[18B] selftest atlandı (fb yok)\n");
        return -1;
    }
    /* Desen 1: zemin + rect + noktalar */
    fb_clear(0x00112233u);
    fb_fill_rect(10, 10, 100, 60, 0x00FF00FFu);
    fb_putpixel(0, 0, 0x00ABCDEFu);
    fb_putpixel(fb_w - 1, fb_h - 1, 0x00123456u);
    fb_flip();
    int ok = 1;
    if (fb_getpixel_front(0, 0) != px_read((uint8_t[]){0xEF,0xCD,0xAB,0x00})) ok = 0;
    if (fb_getpixel_front(50, 40) != px_read((uint8_t[]){0xFF,0x00,0xFF,0x00})) ok = 0;
    if (fb_getpixel_front(200, 200) != px_read((uint8_t[]){0x33,0x22,0x11,0x00})) ok = 0;
    /* Tam kopya doğrulaması: ön == arka */
    if (ok && memcmp(fb_front, fb_back, fb_bytes) != 0) ok = 0;
    /* Desen 2: flip gerçekten güncelliyor mu? */
    fb_clear(0x00000000u);
    fb_flip();
    if (ok && fb_getpixel_front(50, 40) != 0) ok = 0;
    int vb = fb_vblank_wait();
    serial_puts("[18B] vblank "); serial_puts(vb ? "synced\n" : "timeout\n");
    if (ok) {
        serial_puts("[18B] framebuffer flip/readback [PASS]\n");
        vga_puts("[18B] framebuffer flip/readback [PASS]\n");
        return 0;
    }
    serial_puts("[18B] framebuffer [FAIL]\n");
    vga_puts("[18B] framebuffer [FAIL]\n");
    return -1;
}
