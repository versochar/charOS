#include "arch/x86_64/longmode.h"
#include "arch/x86_64/pcihal.h"

#define PCI_MAX 4

typedef struct {
    u64 vendor;
    u64 device;
    int enabled;
} pci_dev_t;

static int pci_inited = 0;
static pci_dev_t devs[PCI_MAX] = {
    {0x8086, 0x100E, 0},
    {0x10EC, 0x8139, 0},
    {0x1AF4, 0x1000, 0},
    {0x1234, 0x1111, 0},
};

int pcihal64_init(void) {
    pci_inited = 1;
    for (int i = 0; i < PCI_MAX; i++) devs[i].enabled = 0;
    return 0;
}

int pcihal64_scan(u64 *out_count) {
    if (!pci_inited || !out_count) return -1;
    *out_count = PCI_MAX;
    return 0;
}

int pcihal64_info(u64 idx, u64 *out_vendor, u64 *out_device) {
    if (!pci_inited || idx >= PCI_MAX || !out_vendor || !out_device) return -1;
    *out_vendor = devs[idx].vendor;
    *out_device = devs[idx].device;
    return 0;
}

int pcihal64_enable(u64 idx) {
    if (!pci_inited || idx >= PCI_MAX) return -1;
    if (devs[idx].enabled) return -1;
    devs[idx].enabled = 1;
    return 0;
}

int pcihal64_read(u64 idx, u64 offset, u64 *out_val) {
    if (!pci_inited || idx >= PCI_MAX || offset > 255 || !out_val) return -1;
    if (!devs[idx].enabled) return -1;
    *out_val = (devs[idx].vendor << 16) | (offset & 0xFF);
    return 0;
}
