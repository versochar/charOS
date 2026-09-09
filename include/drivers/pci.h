#ifndef CHAROS_DRIVERS_PCI_H
#define CHAROS_DRIVERS_PCI_H

#include <stdint.h>

/* 13G: PCI config space (0xCF8/0xCFC) + tarama */

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static inline void pci_outb(uint16_t port, uint8_t v)  { asm volatile("outb %0,%1"::"a"(v),"Nd"(port)); }
static inline uint8_t pci_inb(uint16_t port)           { uint8_t r; asm volatile("inb %1,%0":"=a"(r):"Nd"(port)); return r; }
static inline void pci_outw(uint16_t port, uint16_t v) { asm volatile("outw %0,%1"::"a"(v),"Nd"(port)); }
static inline uint16_t pci_inw(uint16_t port)          { uint16_t r; asm volatile("inw %1,%0":"=a"(r):"Nd"(port)); return r; }
static inline void pci_outl(uint16_t port, uint32_t v) { asm volatile("outl %0,%1"::"a"(v),"Nd"(port)); }
static inline uint32_t pci_inl(uint16_t port)          { uint32_t r; asm volatile("inl %1,%0":"=a"(r):"Nd"(port)); return r; }

/* Config okuma */
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val);

/* vendor/device eşleşen ilk cihazı bul (bus 0 tara) */
int pci_find_device(uint16_t vendor, uint16_t device,
                    uint8_t* bus, uint8_t* slot, uint8_t* func);

/* Command register'da IO space + bus master aç */
void pci_enable_io_bm(uint8_t bus, uint8_t slot, uint8_t func);

/* BAR oku: addr + io flag döner (0 = MMIO) */
uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, int bar, int* is_io);

/* Boot'ta tüm cihazları listele (13G tanılama) */
void pci_init(void);

#endif
