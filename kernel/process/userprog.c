#include <process/task.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* userprog blob (embed_userprog.s'den) */
extern uint8_t userprog_start[];
extern uint8_t userprog_end[];

#define USER_CODE_BASE  0x1000000   /* 16MB */
#define USER_STACK_BASE 0x1400000   /* 20MB - stack aşağı büyür */
#define USER_STACK_SIZE 0x4000      /* 16KB */

/* 13A: ELF veya flat - userprog blob'u ELF ise ELF olarak yükle */
#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'
struct elf32_ehdr_u { uint8_t e_ident[16]; uint16_t e_type; uint16_t e_machine; uint32_t e_version; uint32_t e_entry; uint32_t e_phoff; uint32_t e_shoff; uint32_t e_flags; uint16_t e_ehsize; uint16_t e_phentsize; uint16_t e_phnum; uint16_t e_shentsize; uint16_t e_shnum; uint16_t e_shstrndx; } __attribute__((packed));
struct elf32_phdr_u { uint32_t p_type; uint32_t p_offset; uint32_t p_vaddr; uint32_t p_paddr; uint32_t p_filesz; uint32_t p_memsz; uint32_t p_flags; uint32_t p_align; } __attribute__((packed));

/* Gömülü user programını yükleyip Ring 3 task oluşturur */
struct task* user_launch(void) {
    uint32_t blob_size = (uint32_t)userprog_end - (uint32_t)userprog_start;
    vga_puts("[6B] Loading user program "); vga_putdec(blob_size); vga_puts(" bytes\n");
    serial_puts("[6B] Loading userprog "); serial_puthex(blob_size); serial_puts(" bytes\n");

    struct task* t = task_create_user((void (*)(void))USER_CODE_BASE, "userprog");
    if (!t) return 0;
    uint32_t cr3 = t->cr3;
    uint32_t entry = USER_CODE_BASE;
    struct elf32_ehdr_u* eh = (struct elf32_ehdr_u*)userprog_start;
    int is_elf = (blob_size >= sizeof(struct elf32_ehdr_u) && eh->e_ident[0]==ELF_MAGIC0 && eh->e_ident[1]==ELF_MAGIC1 && eh->e_ident[2]==ELF_MAGIC2 && eh->e_ident[3]==ELF_MAGIC3);
    if (is_elf && eh->e_type == 2 && eh->e_phnum > 0) {
        entry = eh->e_entry;
        for (int i = 0; i < eh->e_phnum; i++) {
            struct elf32_phdr_u* ph = (struct elf32_phdr_u*)(userprog_start + eh->e_phoff + i*eh->e_phentsize);
            if (ph->p_type != 1) continue;
            uint32_t vaddr = ph->p_vaddr;
            uint32_t memsz = ph->p_memsz;
            uint32_t filesz = ph->p_filesz;
            uint32_t offset = ph->p_offset;
            uint32_t pages = (memsz + 0xFFF)/0x1000;
            for (uint32_t p = 0; p < pages; p++) {
                uint32_t va = (vaddr & ~0xFFF) + p*0x1000;
                uint32_t frame = pmm_alloc_frame();
                if (!frame) { vga_puts("[6B] OOM elf\n"); for(;;) asm volatile("cli;hlt"); }
                paging_map_in_space(cr3, va, frame, PAGE_USER | PAGE_RW);
                void* dst = paging_scratch_phys(frame);
                memset(dst, 0, 0x1000);
                uint32_t file_start = offset + p*0x1000;
                uint32_t file_end = offset + filesz;
                if (file_start < file_end) {
                    uint32_t src_off = file_start;
                    uint32_t dst_off = (p==0)? (vaddr & 0xFFF) : 0;
                    uint32_t chunk = 0x1000 - dst_off;
                    if (src_off + chunk > file_end) chunk = file_end - src_off;
                    if (chunk>0) memcpy((char*)dst+dst_off, userprog_start+src_off, chunk);
                }
            }
            task_add_region(t, vaddr & ~0xFFF, pages);
        }
        vga_puts("[13A] ELF loaded entry=0x"); vga_puthex(entry); vga_puts("\n");
        serial_puts("[13A] ELF entry=0x"); serial_puthex(entry); serial_puts("\n");
    } else {
        uint32_t pages = (blob_size + 0xFFF)/0x1000;
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t frame = pmm_alloc_frame();
            if (!frame) { vga_puts("[6B] OOM code\n"); for(;;) asm volatile("cli;hlt"); }
            paging_map_in_space(cr3, USER_CODE_BASE + i*0x1000, frame, PAGE_USER | PAGE_RW);
            uint32_t copy_sz = (i == pages-1) ? (blob_size - i*0x1000) : 0x1000;
            void* dst = paging_scratch_phys(frame);
            memset(dst, 0, 0x1000);
            memcpy(dst, (void*)((uint32_t)userprog_start + i*0x1000), copy_sz);
        }
        task_add_region(t, USER_CODE_BASE, pages);
        entry = USER_CODE_BASE;
    }

    // User stack: 16KB, USER|RW, 20MB'de
    uint32_t stack_pages = USER_STACK_SIZE / 0x1000;
    for (uint32_t i = 0; i < stack_pages; i++) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) { vga_puts("[6B] OOM stack\n"); for(;;) asm volatile("cli;hlt"); }
        paging_map_in_space(cr3, USER_STACK_BASE + i*0x1000, frame, PAGE_USER | PAGE_RW);
        void* dst = paging_scratch_phys(frame);
        memset(dst, 0, 0x1000);
    }
    uint32_t user_esp = USER_STACK_BASE + USER_STACK_SIZE;
    task_add_region(t, USER_STACK_BASE, stack_pages);

    // task_create_user varsayılanları ezip gerçek user state'i doldur
    t->user_esp = user_esp;
    t->user_eip = entry;
    t->user_cs = 0x1B;
    t->user_ss = 0x23;
    t->user_eflags = 0x202;
    t->user_stack_addr = user_esp;

    vga_puts("[12A] userprog isolated CR3=0x"); vga_puthex(cr3); vga_puts("\n");
    serial_puts("[12A] userprog isolated cr3=0x"); serial_puthex(cr3); serial_puts("\n");
    return t;
}
