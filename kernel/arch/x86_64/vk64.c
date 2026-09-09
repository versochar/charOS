/* 47C: Vulkan 1.0 low — GPU listesi + secim + kuyruk sayisi. */
#include "arch/x86_64/longmode.h"

#define VK64_MAX_GPU 4

struct vk64_gpu {
    int used;
    char name[48];
    u64 vram_mb;
    int queues;
};

static struct vk64_gpu vk64_tab[VK64_MAX_GPU];

static void vk_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int vk64_add_gpu(const char *name, u64 vram_mb) {
    int i;
    if (!name || !vram_mb) return -1;
    for (i = 0; i < VK64_MAX_GPU; i++) {
        if (!vk64_tab[i].used) {
            vk64_tab[i].used = 1;
            vk_str_copy(vk64_tab[i].name, name, 48);
            vk64_tab[i].vram_mb = vram_mb;
            vk64_tab[i].queues = (vram_mb >= 512) ? 3 : 1;
            return i;
        }
    }
    return -2;
}

/* En az min_vram saglayan, VRAM'i en kucuk olani sec (verimli). */
int vk64_pick_gpu(u64 min_vram) {
    int i, best = -1;
    for (i = 0; i < VK64_MAX_GPU; i++) {
        if (!vk64_tab[i].used) continue;
        if (vk64_tab[i].vram_mb < min_vram) continue;
        if (best < 0 || vk64_tab[i].vram_mb < vk64_tab[best].vram_mb)
            best = i;
    }
    return best;
}

int vk64_version_check(u32 major, u32 minor) {
    (void)minor;
    if (major < 1) return -1;
    return 0; /* 1.x her zaman (low profil tabani) */
}

int vk64_queue_count(int gpu) {
    if (gpu < 0 || gpu >= VK64_MAX_GPU || !vk64_tab[gpu].used) return -1;
    return vk64_tab[gpu].queues;
}
