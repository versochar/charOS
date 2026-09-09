/* 45G: HID rapor ayrıştırici — oge yuruyucu + klavye ozeti.
 * Kisa ogeler: [bSize:2|bType:2|bTag:4] + veri. Uzun oge (0xFE) atlanir.
 */
#include "arch/x86_64/longmode.h"

int hidparse64_walk(const unsigned char *desc, int len,
                    hidparse64_cb cb) {
    int pos = 0;
    u16 usage_page = 0;
    u16 usages[16];
    int nusage = 0;
    u16 rmin = 0, rmax = 0;
    int have_range = 0;
    u32 report_size = 0;
    u32 report_count = 0;
    u32 bitpos = 0;
    int i;
    if (!desc || len <= 0 || !cb) return -1;
    while (pos < len) {
        unsigned char prefix = desc[pos++];
        unsigned char size, type, tag;
        u32 data = 0;
        int k;
        if (prefix == 0xFE) { /* uzun oge */
            if (pos >= len) return -2;
            size = desc[pos++];
            if (pos + 1 + size > len) return -3;
            pos += 1 + size;
            continue;
        }
        size = prefix & 0x03;
        if (size == 3) size = 4;
        type = (prefix >> 2) & 0x03;
        tag = (prefix >> 4) & 0x0F;
        if (pos + size > len) return -4;
        for (k = 0; k < size; k++) data |= (u32)desc[pos + k] << (k * 8);
        pos += size;
        if (type == 0) { /* ana */
            if (tag == 8) { /* Input */
                u32 j;
                for (i = 0; i < nusage && i < 16; i++) {
                    cb(usage_page, usages[i], bitpos, report_size, 1);
                    bitpos += report_size;
                }
                if (have_range && report_count) {
                    /* Aralik genisletme: count alan, rmin..rmax */
                    for (j = 0; j < report_count; j++) {
                        u16 u = rmin + (u16)j;
                        if (u > rmax) u = rmax;
                        cb(usage_page, u, bitpos, report_size, 1);
                        bitpos += report_size;
                    }
                } else if (!nusage && !have_range && report_size &&
                           report_count) {
                    /* Dolgu/padding alani */
                    bitpos += report_size * report_count;
                }
                nusage = 0;
                have_range = 0;
            } else if (tag == 9) { /* Output */
                nusage = 0;
                have_range = 0;
            } else if (tag == 11) { /* Collection */
                nusage = 0;
                have_range = 0;
            } else if (tag == 12) { /* End collection */
                nusage = 0;
                have_range = 0;
            } else if (tag == 10) { /* Feature */
                nusage = 0;
                have_range = 0;
            }
        } else if (type == 1) { /* global */
            if (tag == 0)
                usage_page = (u16)data;
            else if (tag == 7)
                report_size = data;
            else if (tag == 9)
                report_count = data;
        } else if (type == 2) { /* yerel */
            if (tag == 0 && nusage < 16)
                usages[nusage++] = (u16)data;
            else if (tag == 1) {
                rmin = (u16)data;
                have_range = 1;
            } else if (tag == 2) {
                rmax = (u16)data;
                if (!have_range) {
                    rmin = (u16)data;
                    have_range = 1;
                }
            }
        }
    }
    return 0;
}

struct hidparse64_kb {
    u32 keybits;
    u32 nkeys;
};

static struct hidparse64_kb hidparse64_acc;

/* Klavye ozeti: degistirici-harici 8-bit tus yuvalari.
 * nkeys = yuva adedi (6), keybits = toplam bit (48). */
static void hidparse64_kb_cb(u16 page, u16 usage, u32 bitpos, u32 bits,
                             int is_input) {
    (void)page;
    (void)bitpos;
    if (!is_input || bits != 8) return;
    if (usage >= 0xE0 && usage <= 0xE7) return; /* degistirici */
    hidparse64_acc.nkeys++;
    hidparse64_acc.keybits += bits;
}

int hidparse64_keyboard(const unsigned char *desc, int len, u32 *keybits,
                        u32 *nkeys) {
    hidparse64_acc.keybits = 0;
    hidparse64_acc.nkeys = 0;
    if (hidparse64_walk(desc, len, hidparse64_kb_cb) != 0) return -1;
    if (keybits) *keybits = hidparse64_acc.keybits;
    if (nkeys) *nkeys = hidparse64_acc.nkeys;
    return 0;
}
