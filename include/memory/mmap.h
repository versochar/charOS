#ifndef CHAROS_MEMORY_MMAP_H
#define CHAROS_MEMORY_MMAP_H

#include <stdint.h>
#include <stddef.h>
struct task;

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_ANON MAP_ANONYMOUS

#define MAP_FAILED ((void*)-1)

struct vm_area {
    uint32_t start;
    uint32_t end;
    int prot;
    int flags;
    int fd;
    uint32_t file_offset;
    struct vm_area* next;
};

void mmap_init_task(struct task* t);
void* sys_mmap(uint32_t addr, uint32_t len, int prot, int flags, int fd, uint32_t offset);
int sys_munmap(uint32_t addr, uint32_t len);
int mmap_handle_fault(uint32_t va, int write);

#endif
