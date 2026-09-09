#include <memory/mmap.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <process/task.h>
#include <fs/vfs.h>
#include <fs/chfs.h>
#include <drivers/serial.h>
#include <memory/kheap.h>
#include <string.h>

#define MMAP_BASE 0x3000000  // 48MB - mmap bölgesi başlangıcı
#define MMAP_MAX  0x4000000  // 64MB

static uint32_t mmap_next = MMAP_BASE;

void mmap_init_task(struct task* t) {
    t->vmas = 0;
}

static struct vm_area* find_vma(struct task* t, uint32_t va) {
    struct vm_area* v = t->vmas;
    while (v) {
        if (va >= v->start && va < v->end) return v;
        v = v->next;
    }
    return 0;
}

void* sys_mmap(uint32_t addr, uint32_t len, int prot, int flags, int fd, uint32_t offset) {
    // 13C: global current_task SMP'de yanlış task olabilir; per-CPU kullan
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur || !cur->cr3) return MAP_FAILED;
    if (len == 0) return MAP_FAILED;
    len = (len + 0xFFF) & ~0xFFF;
    // addr hint: eğer MAP_FIXED değilse, mmap_next kullan
    uint32_t start;
    if ((flags & MAP_FIXED) && addr) {
        start = addr & ~0xFFF;
    } else {
        start = mmap_next;
        mmap_next += len;
        if (mmap_next >= MMAP_MAX) mmap_next = MMAP_BASE;
        // çakışma kontrolü (basit)
        for (int i = 0; i < 8; i++) {
            struct vm_area* v = cur->vmas;
            int overlap = 0;
            while (v) {
                if (!(start+len <= v->start || start >= v->end)) { overlap = 1; break; }
                v = v->next;
            }
            if (!overlap) break;
            start += 0x100000; // 1MB ileri
            if (start + len >= MMAP_MAX) return MAP_FAILED;
        }
    }
    // VMA oluştur
    struct vm_area* vma = (struct vm_area*)kmalloc(sizeof(struct vm_area));
    if (!vma) return MAP_FAILED;
    vma->start = start;
    vma->end = start + len;
    vma->prot = prot;
    vma->flags = flags;
    vma->fd = fd;
    vma->file_offset = offset;
    vma->next = cur->vmas;
    cur->vmas = vma;

    (void)prot;
    // Lazy: hemen map etme, fault'ta alloc edilecek
    // Hemen map için loop'u aç:
    // for (uint32_t va = start; va < start+len; va+=0x1000) { uint32_t f=pmm_alloc_frame(); paging_map_in_space(cur->cr3, va, f, PAGE_USER|PAGE_RW); task_add_region(cur, va,1); }

    // 13C: task_add_region ile VMA'yı da takip et (demand için)
    // VMA zaten var, fault handler VMA'ya bakacak
    // Ayrıca user_regions'e ekle ki fork CoW bilsin
    for (uint32_t va = start; va < start+len; va+=0x1000) {
        // Lazy: sadece VMA, mapping yok. Fork için region ekle
    }
    // Fork için region olarak ekle (demand)
    task_add_region(cur, start, len/0x1000);

    serial_puts("[13C] mmap "); serial_puthex(start); serial_puts(" len "); serial_puthex(len); serial_puts(" prot "); serial_puthex(prot); serial_puts(flags & MAP_SHARED ? " SHARED\n" : " PRIVATE\n");
    return (void*)start;
}

int sys_munmap(uint32_t addr, uint32_t len) {
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur) return -1;
    addr &= ~0xFFF;
    len = (len + 0xFFF) & ~0xFFF;
    struct vm_area *prev = 0, *cur_vma = cur->vmas;
    while (cur_vma) {
        if (cur_vma->start == addr && cur_vma->end == addr+len) {
            // Unmap sayfaları
            for (uint32_t va = addr; va < addr+len; va+=0x1000) {
                uint32_t phys = paging_get_phys(va);
                if (phys) {
                    pmm_free_frame(phys & ~0xFFF);
                    paging_unmap_in_space(cur->cr3, va);
                }
            }
            if (prev) prev->next = cur_vma->next;
            else cur->vmas = cur_vma->next;
            kfree(cur_vma);
            // user_regions'ten de çıkar (basit: bırak)
            return 0;
        }
        prev = cur_vma;
        cur_vma = cur_vma->next;
    }
    return -1;
}

int mmap_handle_fault(uint32_t va, int write) {
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur || !cur->cr3) return 0;
    struct vm_area* vma = find_vma(cur, va & ~0xFFF);
    if (!vma) return 0;
    // prot check
    if (write && !(vma->prot & PROT_WRITE)) return 0;
    uint32_t page_va = va & ~0xFFF;
    int page_flags = PAGE_USER;
    if (vma->prot & PROT_WRITE) page_flags |= PAGE_RW;
    // File-backed ise dosyadan oku
    if (!(vma->flags & MAP_ANONYMOUS) && vma->fd >= 0) {
        // File-backed: fd'den oku
        // Basit: chfs_read_file ile offset'ten oku
        // VMA'nın file_offset + (va - vma->start) kadar file offset
        // File size kontrolü için chfs_get_size kullan
        // Şimdilik anon gibi sıfırla ve file'dan kopya
        uint32_t frame = pmm_alloc_frame();
        if (!frame) return 0;
        void* dst = paging_scratch_phys(frame);
        memset(dst, 0, 0x1000);
        // File'dan oku
        if (vma->fd >= 0) {
            // File descriptor'dan offset ile oku
            // Basit: chfs_read_file ile tüm file'ı oku ve kopya
            // File offset: vma->file_offset + (va - vma->start)
            uint32_t file_off = vma->file_offset + (page_va - vma->start);
            // File'ı oku - chfs üzerinden
            // File size kadar kopya
            // Basit: block_pool'dan direk kopya (chfs internal)
            extern char block_pool[FS_MAX_BLOCKS][FS_BLOCK_SIZE];
            extern struct inode inode_table[FS_MAX_FILES];
            extern int inode_used[FS_MAX_FILES];
            // fd'den inode bul
            // cur->fd_table[vma->fd] valid mi?
            if (vma->fd >= 0 && vma->fd < 16 && cur->fd_table[vma->fd].valid) {
                int ino = cur->fd_table[vma->fd].inode_id;
                if (ino >= 0 && ino < FS_MAX_FILES && inode_used[ino]) {
                    int blk_idx = file_off / 512;
                    int blk_off = file_off % 512;
                    if (blk_idx < CHFS_DATABLOCKS) {
                        int blk = inode_table[ino].blocks[blk_idx];
                        if (blk >= 0 && blk < FS_MAX_BLOCKS) {
                            int chunk = 512 - blk_off;
                            if (chunk > 4096) chunk = 4096;
                            memcpy(dst, &block_pool[blk][blk_off], chunk < 4096 ? chunk : 4096);
                        }
                    }
                }
            }
        }
        paging_map_in_space(cur->cr3, page_va, frame, page_flags);
        return 1;
    } else {
        // Anon: sıfır page
        // Shared vs Private: Shared için aynı frame'i paylaşacak (fork sonrası CoW ile)
        // Şimdilik her fault'ta yeni frame
        uint32_t frame = pmm_alloc_frame();
        if (!frame) return 0;
        void* dst = paging_scratch_phys(frame);
        memset(dst, 0, 0x1000);
        paging_map_in_space(cur->cr3, page_va, frame, page_flags);
        return 1;
    }
    return 0;
}
