#ifndef VFS_H
#define VFS_H

#include "arch/x86_64/longmode.h"

int vfs64_init(void);
int vfs64_create(u64 *out_ino);
int vfs64_unlink(u64 ino);
int vfs64_open(u64 ino, u64 *out_fd);
int vfs64_close(u64 fd);
int vfs64_write(u64 fd, u64 val);
int vfs64_read(u64 fd, u64 *out_val);

#endif
