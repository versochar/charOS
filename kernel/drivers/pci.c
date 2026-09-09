#include <drivers/pci.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 13G: PCI config erişimi + bus tarama */

static uint32_t pci_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
           ((uint32_t)func << 8) | (offset & 0xFCu);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    pci_outl(PCI_CONFIG_ADDR, pci_addr(bus, slot, func, offset));
    return pci_inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    /* 16-bit hizalı oku: offset'in alt 2 biti 0/2 olabilir */
    uint32_t v = pci_config_read32(bus, slot, func, offset & 0xFCu);
    if (offset & 2) return (uint16_t)(v >> 16);
    return (uint16_t)(v & 0xFFFFu);
}

void pci_config_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    pci_outl(PCI_CONFIG_ADDR, pci_addr(bus, slot, func, offset));
    pci_outl(PCI_CONFIG_DATA, val);
}

void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    uint32_t cur = pci_config_read32(bus, slot, func, offset & 0xFCu);
    if (offset & 2) cur = (cur & 0xFFFFu) | ((uint32_t)val << 16);
    else cur = (cur & 0xFFFF0000u) | val;
    pci_config_write32(bus, slot, func, offset & 0xFCu, cur);
}

static void pci_check_slot(uint8_t bus, uint8_t slot, uint16_t vendor, uint16_t device,
                           int each_func, uint8_t* ob, uint8_t* os, uint8_t* of, int* found) {
    int nfunc = each_func ? 8 : 1;
    for (int f = 0; f < nfunc && !*found; f++) {
        uint32_t id = pci_config_read32(bus, slot, f, 0x00);
        if ((id & 0xFFFFu) == 0xFFFFu) continue; /* cihaz yok */
        if ((uint16_t)(id & 0xFFFFu) == vendor && (uint16_t)(id >> 16) == device) {
            *ob = bus; *os = slot; *of = f; *found = 1;
        }
    }
}

int pci_find_device(uint16_t vendor, uint16_t device,
                    uint8_t* bus, uint8_t* slot, uint8_t* func) {
    int found = 0;
    for (uint8_t b = 0; b < 8 && !found; b++) {
        for (uint8_t s = 0; s < 32 && !found; s++) {
            uint32_t id0 = pci_config_read32(b, s, 0, 0x00);
            if ((id0 & 0xFFFFu) == 0xFFFFu) continue;
            uint32_t hdr = pci_config_read32(b, s, 0, 0x0C);
            int multi = (hdr >> 16) & 0x80u; /* header type bit7 */
            pci_check_slot(b, s, vendor, device, multi ? 1 : 0, bus, slot, func, &found);
        }
    }
    return found ? 0 : -1;
}

void pci_enable_io_bm(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t cmd = pci_config_read16(bus, slot, func, 0x04);
    cmd |= 0x07u; /* bit0 IO space + bit1 Memory space + bit2 bus master */
    pci_config_write16(bus, slot, func, 0x04, cmd);
}

uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, int bar, int* is_io) {
    uint32_t v = pci_config_read32(bus, slot, func, 0x10 + bar * 4);
    if (v & 1u) {
        if (is_io) *is_io = 1;
        return v & ~0x3u;
    }
    if (is_io) *is_io = 0;
    return v & ~0xFu;
}

void pci_init(void) {
    int count = 0;
    serial_puts("[13G] PCI scan (bus 0-7):\n");
    for (uint8_t b = 0; b < 8; b++) {
        for (uint8_t s = 0; s < 32; s++) {
            uint32_t id0 = pci_config_read32(b, s, 0, 0x00);
            if ((id0 & 0xFFFFu) == 0xFFFFu) continue;
            uint32_t hdr = pci_config_read32(b, s, 0, 0x0C);
            int multi = (hdr >> 16) & 0x80u;
            int nfunc = multi ? 8 : 1;
            for (int f = 0; f < nfunc; f++) {
                uint32_t id = pci_config_read32(b, s, f, 0x00);
                if ((id & 0xFFFFu) == 0xFFFFu) continue;
                uint32_t cls = pci_config_read32(b, s, f, 0x08);
                serial_puts("  dev "); serial_puthex(b);
                serial_puts(":"); serial_puthex(s);
                serial_puts("."); serial_puthex(f);
                serial_puts(" vend="); serial_puthex(id & 0xFFFFu);
                serial_puts(" dev="); serial_puthex(id >> 16);
                serial_puts(" class="); serial_puthex(cls >> 16);
                serial_puts("\n");
                count++;
            }
        }
    }
    vga_puts("[13G] PCI: "); vga_putdec(count); vga_puts(" device(s)\n");
    serial_puts("[13G] PCI devices="); serial_puthex(count); serial_puts("\n");
}
