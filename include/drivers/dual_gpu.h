#ifndef CHAROS_DRIVERS_DUAL_GPU_H
#define CHAROS_DRIVERS_DUAL_GPU_H

#include <stdint.h>

/* 29B: Dual-GPU temel (Intel Iris Xe + NVIDIA MX350 takas/GOP).
 * Gerçek GPU takas (Intel -> NVIDIA) karmaşık (GOP, VBIOS, PCI BAR);
 * skeleton: her iki GPU'yu tespit et + GOP framebuffer kullan. */

/* GPU tür tanımları */
#define GPU_TYPE_NONE 0
#define GPU_TYPE_INTEL 1
#define GPU_TYPE_NVIDIA 2

/* Dual-GPU başlatma (PCI tarama + GOP) */
int dual_gpu_init(void);

/* Aktif GPU'yu seç (Intel -> NVIDIA geçişi skeleton) */
int dual_gpu_select(int gpu_type);  /* 1=Intel, 2=NVIDIA */

/* Self-test: GPU tespiti + GOP durumu */
int dual_gpu_selftest(void);

#endif
