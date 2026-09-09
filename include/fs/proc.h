#ifndef CHAROS_FS_PROC_H
#define CHAROS_FS_PROC_H

/* 9A: /proc pseudo-dosya sistemi (basit) */
void proc_init(void);
int proc_read_pid(const char* path, char* buf, int len);
/* 14F: /proc v2 — status/mem/fds/uptime (dönen: bayt / -1 yok) */
int proc_read(const char* path, char* buf, int len);
/* 20B: sanal dosya dağıtıcısı (/proc /sys /dev) */
int pfs_open(const char* path);
int pfs_read(const char* path, int off, char* buf, int len);
int pfs_write(const char* path, const char* buf, int len);

#endif
