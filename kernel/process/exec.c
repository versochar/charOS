#include <process/exec.h>
#include <process/task.h>
#include <memory/paging.h>
#include <memory/mmap.h>
#include <memory/pmm.h>
#include <memory/kheap.h>
#include <fs/vfs.h>
#include <fs/chfs.h>
#include <fs/fd.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <core/isr.h>
#include <core/syscall.h>
#include <string.h>

#define USER_CODE_BASE  0x1000000
#define USER_STACK_BASE 0x1400000
#define USER_STACK_SIZE 0x4000

// 13A: ELF32
#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
#define ELF_CLASS_32 1
#define ELF_DATA_2LSB 1
#define ET_EXEC 2
#define PT_LOAD 1
struct elf32_ehdr {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));
struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed));

int exec_flat(const char* path, const char* args) {
    if (!path) return -1;
    int sz = chfs_get_size(path);
    if (sz < 0) {
        vga_puts("[12C] exec: file not found "); vga_puts(path); vga_puts("\n");
        serial_puts("[12C] exec not found "); serial_puts(path); serial_puts("\n");
        return -1;
    }
    if (sz > CHFS_DATABLOCKS * 512) {
        vga_puts("[12C] exec: file too large\n");
        return -1;
    }
    /* 19A: argümanları adres alanı yıkılmadan ÖNCE token'la + ölç (kargs kernel heap'te) */
    const char* tok[15]; /* argv[0]=path, en fazla 15 ek argüman */
    int ntok = 0;
    uint32_t strneed = 0;
    {
        int plen = strlen(path) + 1;
        strneed = (plen + 3) & ~3u;
        if (args) {
            const char* p = args;
            while (*p && ntok < 15) {
                while (*p == ' ') p++;
                if (!*p) break;
                const char* s = p;
                while (*p && *p != ' ') p++;
                tok[ntok++] = s;
                /* uzunluğu şimdiden hesapla (NUL sınırı p'de) */
                int L = 0;
                const char* q = s;
                while (q < p) { L++; q++; }
                strneed += (L + 1 + 3) & ~3u;
            }
            if (*p) {
                while (*p == ' ') p++;
                if (*p) return -1; /* 15'ten fazla argüman */
            }
        }
    }
    int argc19 = 1 + ntok;
    uint32_t need = 4 + (argc19 + 1) * 4 + strneed;
    if (need > 480) return -1;
    /* token sonlarını kopyala (stack'e yazarken NUL bitimleri gerekir) */
    char tokbuf[15][64];
    for (int i = 0; i < ntok; i++) {
        const char* s = tok[i];
        int L = 0;
        while (s[L] && s[L] != ' ') L++;
        if (L == 0 || L >= 64) return -1; /* boş/devasa token */
        for (int k = 0; k < L; k++) tokbuf[i][k] = s[k];
        tokbuf[i][L] = '\0';
        tok[i] = tokbuf[i];
    }
    struct task* cur = task_current();
    if (!cur || !cur->cr3) return -1;

    uint32_t cr3 = cur->cr3;

    // 13C: VMA'ları temizle (stack hariç)
    struct vm_area* v = cur->vmas;
    while (v) {
        struct vm_area* nxt = v->next;
        if (v->start != USER_STACK_BASE) {
            for (uint32_t va = v->start; va < v->end; va+=0x1000) {
                uint32_t phys = paging_get_phys(va);
                if (phys) pmm_free_frame(phys & ~0xFFF);
                paging_unmap_in_space(cr3, va);
            }
            kfree(v);
        } else {
            // stack VMA'sını koru ama içini sıfırlayacağız
        }
        v = nxt;
    }
    // Stack VMA'sını bul ve koru
    struct vm_area* stack_vma = 0;
    v = cur->vmas;
    while (v) { if (v->start == USER_STACK_BASE) { stack_vma = v; break; } v = v->next; }
    // Tüm VMA listesini temizle ve sadece stack'i bırak
    v = cur->vmas;
    while (v) { struct vm_area* nxt = v->next; if (v != stack_vma) kfree(v); v = nxt; }
    cur->vmas = stack_vma;
    if (stack_vma) stack_vma->next = 0;
    else {
        // Stack VMA yoksa oluştur (demand için)
        stack_vma = (struct vm_area*)kmalloc(sizeof(struct vm_area));
        if (stack_vma) {
            stack_vma->start = USER_STACK_BASE;
            stack_vma->end = USER_STACK_BASE + USER_STACK_SIZE;
            stack_vma->prot = PROT_READ | PROT_WRITE;
            stack_vma->flags = MAP_PRIVATE | MAP_ANONYMOUS;
            stack_vma->fd = -1;
            stack_vma->next = 0;
            cur->vmas = stack_vma;
        }
    }
    // 12C: Eski user code'u temizle
    for (int r = 0; r < cur->user_region_count; r++) {
        uint32_t base = cur->user_regions[r].base;
        uint32_t pages = cur->user_regions[r].pages;
        if (base == USER_STACK_BASE) continue;
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t vaddr = base + i * 0x1000;
            uint32_t phys = paging_get_phys(vaddr);
            if (phys) pmm_free_frame(phys & ~0xFFF);
            paging_unmap_in_space(cr3, vaddr);
        }
    }
    int new_count = 0;
    for (int r = 0; r < cur->user_region_count; r++) {
        if (cur->user_regions[r].base == USER_STACK_BASE) {
            cur->user_regions[new_count++] = cur->user_regions[r];
        }
    }
    cur->user_region_count = new_count;

    // 13A: ELF veya flat - file'ı önce kernel heap'e oku ve magic'e bak
    char* file_buf = (char*)kmalloc(sz);
    if (!file_buf) return -1;
    if (chfs_read_file(path, file_buf, sz) < 0) { kfree(file_buf); return -1; }
    struct elf32_ehdr* eh = (struct elf32_ehdr*)file_buf;
    int is_elf = ((uint32_t)sz >= sizeof(struct elf32_ehdr) &&
                  eh->e_ident[0] == ELF_MAGIC0 && eh->e_ident[1] == ELF_MAGIC1 &&
                  eh->e_ident[2] == ELF_MAGIC2 && eh->e_ident[3] == ELF_MAGIC3 &&
                  eh->e_ident[4] == ELF_CLASS_32);
    uint32_t entry = USER_CODE_BASE;
    if (is_elf) {
        if (eh->e_type != ET_EXEC || eh->e_phnum == 0) { kfree(file_buf); return -1; }
        entry = eh->e_entry;
        // Her PT_LOAD segmentini yükle
        for (int i = 0; i < eh->e_phnum; i++) {
            struct elf32_phdr* ph = (struct elf32_phdr*)(file_buf + eh->e_phoff + i*eh->e_phentsize);
            if (ph->p_type != PT_LOAD) continue;
            uint32_t vaddr = ph->p_vaddr;
            uint32_t memsz = ph->p_memsz;
            uint32_t filesz = ph->p_filesz;
            uint32_t offset = ph->p_offset;
            uint32_t pages = (memsz + 0xFFF) / 0x1000;
            for (uint32_t p = 0; p < pages; p++) {
                uint32_t va = (vaddr & ~0xFFF) + p*0x1000;
                uint32_t frame = pmm_alloc_frame();
                if (!frame) { kfree(file_buf); return -1; }
                paging_map_in_space(cr3, va, frame, PAGE_USER | PAGE_RW);
                void* dst = paging_scratch_phys(frame);
                memset(dst, 0, 0x1000);
                uint32_t file_start = offset + p*0x1000;
                uint32_t file_end = offset + filesz;
                if (file_start < file_end) {
                    uint32_t src_off = file_start;
                    uint32_t dst_off = (p==0) ? (vaddr & 0xFFF) : 0;
                    uint32_t chunk = 0x1000 - dst_off;
                    if (src_off + chunk > file_end) chunk = file_end - src_off;
                    if (chunk > 0) memcpy((char*)dst + dst_off, file_buf + src_off, chunk);
                }
            }
            task_add_region(cur, vaddr & ~0xFFF, pages);
        }
        kfree(file_buf);
    } else {
        uint32_t pages = (sz + 0xFFF) / 0x1000;
        if (pages == 0) pages = 1;
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t frame = pmm_alloc_frame();
            if (!frame) { kfree(file_buf); return -1; }
            paging_map_in_space(cr3, USER_CODE_BASE + i*0x1000, frame, PAGE_USER | PAGE_RW);
            void* dst = paging_scratch_phys(frame);
            memset(dst, 0, 0x1000);
            int file_off = i * 0x1000;
            int copy_len = sz - file_off;
            if (copy_len > 0x1000) copy_len = 0x1000;
            if (copy_len < 0) copy_len = 0;
            if (copy_len > 0) memcpy(dst, file_buf + file_off, copy_len);
        }
        kfree(file_buf);
        task_add_region(cur, USER_CODE_BASE, pages);
        entry = USER_CODE_BASE;
    }

    // Stack'i yeniden kur (4 sayfa, sıfırla).
    // KÖK NEDEN FIX: eski kod mevcut çerçeveyi scratch üzerinden yerinde
    // sıfırlıyordu. Fork sonrası stack çerçeveleri parent ile CoW paylaşımlı
    // olabilir; child'ın exec'i burada o PAYLAŞIMLI fiziksel sayfayı silip
    // parent'ın canlı stack'ini (lokaller/canary/dönüş adresleri) bozuyordu
    // ("[19A] canary clobbered"). Çözüm: her stack sayfasına DAİMA TAZE çerçeve
    // al, eskiyi pmm_free_frame ile bırak (refcount>1 ise yalnızca ref düşer,
    // parent'ın kopyası korunur).
    uint32_t stack_pages = USER_STACK_SIZE / 0x1000;
    for (uint32_t i = 0; i < stack_pages; i++) {
        uint32_t vaddr = USER_STACK_BASE + i*0x1000;
        uint32_t phys = paging_get_phys(vaddr);
        uint32_t frame = pmm_alloc_frame();
        if (!frame) return -1;
        if (phys) pmm_free_frame(phys & ~0xFFF);
        paging_map_in_space(cr3, vaddr, frame, PAGE_USER | PAGE_RW);
        void* dst = paging_scratch_phys(frame);
        memset(dst, 0, 0x1000);
    }
    // Stack region'ı ekle (eğer yoksa)
    int has_stack = 0;
    for (int i = 0; i < cur->user_region_count; i++) {
        if (cur->user_regions[i].base == USER_STACK_BASE) { has_stack = 1; break; }
    }
    if (!has_stack) task_add_region(cur, USER_STACK_BASE, stack_pages);

    uint32_t new_esp = USER_STACK_BASE + USER_STACK_SIZE;
    /* 19A: argv bloğu — [argc][argv0..][NULL][stringler], esp=argc adresi.
     * Stack sayfaları yukarıda sıfırlandı; en üst sayfaya yazıyoruz. */
    {
        uint32_t total = 4 + (argc19 + 1) * 4 + strneed;
        total = (total + 15) & ~15u; /* 16B hizalı */
        uint32_t base = USER_STACK_BASE + USER_STACK_SIZE - total;
        uint32_t* w = (uint32_t*)base;
        w[0] = (uint32_t)argc19;
        uint32_t* slots = w + 1; /* argv[0..argc-1], sonra NULL */
        char* sp = (char*)(base + 4 + (argc19 + 1) * 4);
        /* argv[0] = path */
        {
            int L = strlen(path) + 1;
            memcpy(sp, path, L);
            slots[0] = (uint32_t)sp;
            sp += (L + 3) & ~3u;
        }
        for (int i = 0; i < ntok; i++) {
            int L = strlen(tok[i]) + 1;
            memcpy(sp, tok[i], L);
            slots[1 + i] = (uint32_t)sp;
            sp += (L + 3) & ~3u;
        }
        slots[argc19] = 0;
        new_esp = base;
    }
    cur->user_eip = entry;
    cur->user_esp = new_esp;
    cur->user_stack_addr = new_esp;
    cur->user_cs = 0x1B;
    cur->user_ss = 0x23;
    cur->user_eflags = 0x202;

    /* 20A: task adını exec edilen programa güncelle (ps çıktısında yansır).
     * fork_child gibi jenerik adlar üzerine path'in son bileşeni gelir. */
    {
        const char* bn = path;
        for (int i = 0; path[i]; i++) if (path[i] == '/') bn = path + i + 1;
        int bnl = 0;
        while (bn[bnl]) bnl++;
        if (bnl > 0 && bnl < TASK_NAME_LEN) {
            for (int i = 0; i < bnl; i++) cur->name[i] = bn[i];
            cur->name[bnl] = 0;
        }
    }

    // Trap frame'i güncelle (syscall'dan dönerken yeni eip/esp'ye gidecek)
    struct registers* regs = syscall_get_regs();
    if (regs) {
        regs->eip = entry;
        regs->useresp = new_esp;
        regs->cs = 0x1B;
        regs->ss = 0x23;
        regs->eflags = 0x202;
        regs->eax = 0; // exec success
    }

    vga_puts("[12C] exec ok "); vga_puts(path); vga_puts(" -> "); vga_puthex(USER_CODE_BASE); vga_puts("\n");
    serial_puts("[12C] exec ok "); serial_puts(path); serial_puts("\n");
    return 0;
}
