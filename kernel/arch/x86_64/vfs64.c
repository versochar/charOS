#include "arch/x86_64/longmode.h"
#include "arch/x86_64/vfs.h"

#define VFS_INODES 16
#define VFS_FDS 32

typedef struct {
    u64 ino;
    u64 val;
    int active;
} vfs_inode_t;

typedef struct {
    u64 fd;
    u64 ino;
    int active;
} vfs_fd_t;

static int vfs_inited = 0;
static u64 ino_ctr = 0;
static u64 fd_ctr = 0;
static vfs_inode_t inodes[VFS_INODES];
static vfs_fd_t fds[VFS_FDS];

static vfs_inode_t *find_ino(u64 ino) {
    for (int i = 0; i < VFS_INODES; i++) {
        if (inodes[i].active && inodes[i].ino == ino) return &inodes[i];
    }
    return 0;
}

static vfs_fd_t *find_fd(u64 fd) {
    for (int i = 0; i < VFS_FDS; i++) {
        if (fds[i].active && fds[i].fd == fd) return &fds[i];
    }
    return 0;
}

static int ino_busy(u64 ino) {
    for (int i = 0; i < VFS_FDS; i++) {
        if (fds[i].active && fds[i].ino == ino) return 1;
    }
    return 0;
}

int vfs64_init(void) {
    vfs_inited = 1;
    ino_ctr = 0; fd_ctr = 0;
    for (int i = 0; i < VFS_INODES; i++) { inodes[i].ino = 0; inodes[i].active = 0; }
    for (int i = 0; i < VFS_FDS; i++) { fds[i].fd = 0; fds[i].active = 0; }
    return 0;
}

int vfs64_create(u64 *out_ino) {
    if (!vfs_inited || !out_ino) return -1;
    for (int i = 0; i < VFS_INODES; i++) {
        if (!inodes[i].active) {
            ino_ctr++;
            inodes[i].ino = ino_ctr;
            inodes[i].val = 0;
            inodes[i].active = 1;
            *out_ino = ino_ctr;
            return 0;
        }
    }
    return -1;
}

int vfs64_unlink(u64 ino) {
    vfs_inode_t *n = find_ino(ino);
    if (!n) return -1;
    if (ino_busy(ino)) return -2;
    n->active = 0; n->ino = 0;
    return 0;
}

int vfs64_open(u64 ino, u64 *out_fd) {
    if (!find_ino(ino) || !out_fd) return -1;
    for (int i = 0; i < VFS_FDS; i++) {
        if (!fds[i].active) {
            fd_ctr++;
            fds[i].fd = fd_ctr;
            fds[i].ino = ino;
            fds[i].active = 1;
            *out_fd = fd_ctr;
            return 0;
        }
    }
    return -1;
}

int vfs64_close(u64 fd) {
    vfs_fd_t *f = find_fd(fd);
    if (!f) return -1;
    f->active = 0; f->fd = 0;
    return 0;
}

int vfs64_write(u64 fd, u64 val) {
    vfs_fd_t *f = find_fd(fd);
    if (!f) return -1;
    vfs_inode_t *n = find_ino(f->ino);
    if (!n) return -1;
    n->val = val;
    return 0;
}

int vfs64_read(u64 fd, u64 *out_val) {
    vfs_fd_t *f = find_fd(fd);
    if (!f || !out_val) return -1;
    vfs_inode_t *n = find_ino(f->ino);
    if (!n) return -1;
    *out_val = n->val;
    return 0;
}
