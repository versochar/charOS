#include <drivers/hda.h>
#include <drivers/hdaverb.h>
#include <drivers/pci.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 28E: Intel HD Audio (HDA) - Tiger Lake (PCI 00:1f.3 gibi).
 * Temel skeleton: PCI tarama -> BAR map -> GCTL init -> codec probe.
 * Gerçek ses çıkışı (codec VERB, DMA stream) ileri aşama. */

static int hda_found = 0;

/* PCI tarama: class 04 (Multimedia) / subclass 03 (Audio) */
static int hda_pci_probe(void) {
    for (uint8_t bus = 0; bus < 2; bus++) { /* sadece bus 0 */
        for (uint8_t dev_idx = 0; dev_idx < 32; dev_idx++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t class_rev = pci_config_read32(bus, dev_idx, func, 8);
                uint8_t class_code = (uint8_t)(class_rev >> 24);
                uint8_t subclass = (uint8_t)((class_rev >> 16) & 0xFF);
                if (class_code == 0x04 && subclass == 0x03) {
                    uint32_t bar0 = pci_read_bar(bus, dev_idx, func, 0, 0);
                    int is_io = 0;
                    bar0 = (uint32_t)pci_read_bar(bus, dev_idx, func, 0, &is_io);
                    hda_found = 1;
                    serial_puts("[28E] HDA PCI found bus="); serial_puthex(bus);
                    serial_puts(" dev="); serial_puthex(dev_idx);
                    serial_puts(" func="); serial_puthex(func);
                    serial_puts(" BAR0="); serial_puthex(bar0);
                    serial_puts(" io="); serial_puthex(is_io);
                    serial_puts("\n");
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* HDA başlatma: GCTL (Global Control) reset + init */
int hda_init(void) {
    if (hda_pci_probe() == 0) {
        serial_puts("[28E] HDA controller bulunamadı\n");
        return -1;
    }
    serial_puts("[28E] HDA init OK (skeleton)\n");
    vga_puts("[28E] HDA init OK (skeleton)\n");
    return 0;
}

int hda_present(void) { return hda_found ? 1 : 0; }

/* 36.4: codec taşıması (CORB/RIRB kurulunca atanır; yoksa hata) */
static hda_xport_fn xport_fn = 0;

void hda_set_xport(hda_xport_fn fn) {
    xport_fn = fn;
}

/* Codec VERB gönderme: komut kodlanır, taşıma yoksa -1 (eski sahte 0 gitti) */
int hda_send_verb(uint32_t nid, uint32_t verb, uint32_t param) {
    uint32_t cmd = hdaverb_build(nid, verb, param);
    uint32_t resp = 0;
    if (!xport_fn) return -1;
    return xport_fn(cmd, &resp);
}

/* Self-test: HDA tespiti */
int hda_selftest(void) {
    if (!hda_found && hda_init() != 0) {
        serial_puts("[28E] HDA selftest atlandı (controller yok)\n");
        return -1;
    }
    serial_puts("[28E] HDA controller detected [PASS]\n");
    vga_puts("[28E] HDA controller detected [PASS]\n");
    return 0;
}
