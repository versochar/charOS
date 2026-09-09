#ifndef CHAROS_FS_FD_H
#define CHAROS_FS_FD_H

#include <stdint.h>

#define FD_NONE 0
#define FD_FILE 1
#define FD_PIPE 2
#define FD_CONSOLE 3 // stdin/stdout/stderr (12D: OPEN bunları vermez)
#define FD_PSEUDO 4 // /proc /sys /dev sanal dosyalar (20B)

struct pipe; // forward

struct fd_entry {
    int valid;
    int type; // FD_NONE/FILE/PIPE/CONSOLE/PSEUDO
    int inode_id; // for files
    int mode;
    int offset;
    struct pipe* pipe; // for pipes
    int pipe_write; // 1=write end, 0=read end
    char pseudo[32]; // for pseudo files: "/proc/status", "/dev/null", ...
};

#endif
