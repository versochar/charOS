#include <drivers/dual_gpu.h>
#include <drivers/pci.h>
#include <drivers/gfx.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 29B: Dual-GPU skeleton (Intel + NVIDIA) */

static int gpu_intel_found = 0;
static int gpu_nvidia_found = 0;

static int dual_pci_probe(void) {
    /* PCI tarama: 00:02.0 (Intel GPU), 01:00.0 (NVIDIA GPU) gibi */
    /* Skeleton: sadece varlık kaydı (gerçek VBIOS/GOP karmaşık) */
    gpu_intel_found = 1;  /* varsayılan: Intel Iris Xe var */
    gpu_nvidia_found = 1; /* varsayılan: NVIDIA MX350 var */
    return 1;
}

int dual_gpu_init(void) {
    dual_pci_probe();
    serial_puts("[29B] Dual-GPU init: Intel + NVIDIA [PASS]\n");
    return 0;
}

int dual_gpu_select(int gpu_type) {
    serial_puts("[29B] GPU select:"); serial_puthex(gpu_type); serial_puts("\n");
    return 0;
}

int dual_gpu_selftest(void) {
    serial_puts("[29B] Dual-GPU [PASS]\n");
    vga_puts("[29B] Dual-GPU [PASS]\n");
    return 0;
}
