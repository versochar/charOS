#include <fs/proc.h>
#include <process/task.h>
#include <memory/pmm.h>
#include <drivers/timer.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14F: küçük yazıcılar (snprintf yok) */
static void proc_u32(char* buf, int* pos, int max, uint32_t v) {
    char rev[11];
    int r = 0;
    if (v == 0) rev[r++] = '0';
    while (v && r < 10) { rev[r++] = '0' + (v % 10); v /= 10; }
    while (r > 0 && *pos < max - 1) buf[(*pos)++] = rev[--r];
}

static void proc_str(char* buf, int* pos, int max, const char* s) {
    while (*s && *pos < max - 1) buf[(*pos)++] = *s++;
}

int proc_read(const char* path, char* buf, int len) {
    if (!path || !buf || len <= 0) return -1;
    struct task* cur = task_current();
    if (!cur) return -1;
    int pos = 0;
    if (strcmp(path, "status") == 0) {
        proc_str(buf, &pos, len, "pid=");
        proc_u32(buf, &pos, len, (uint32_t)cur->pid);
        proc_str(buf, &pos, len, " uid=");
        proc_u32(buf, &pos, len, (uint32_t)cur->uid);
        proc_str(buf, &pos, len, " gid=");
        proc_u32(buf, &pos, len, (uint32_t)cur->gid);
        proc_str(buf, &pos, len, " state=");
        proc_u32(buf, &pos, len, (uint32_t)cur->state);
        proc_str(buf, &pos, len, " prio=");
        proc_u32(buf, &pos, len, (uint32_t)cur->priority);
        proc_str(buf, &pos, len, "\n");
    } else if (strcmp(path, "mem") == 0) {
        proc_str(buf, &pos, len, "pmm_free=");
        proc_u32(buf, &pos, len, pmm_get_free_count());
        proc_str(buf, &pos, len, "\n");
    } else if (strcmp(path, "fds") == 0) {
        for (int i = 0; i < 16 && pos < len - 8; i++) {
            if (!cur->fd_table[i].valid) continue;
            proc_u32(buf, &pos, len, (uint32_t)i);
            proc_str(buf, &pos, len, ":");
            proc_u32(buf, &pos, len, (uint32_t)cur->fd_table[i].type);
            proc_str(buf, &pos, len, "\n");
        }
    } else if (strcmp(path, "uptime") == 0) {
        proc_u32(buf, &pos, len, timer_get_ticks() / 100);
        proc_str(buf, &pos, len, "\n");
    } else if (strcmp(path, "version") == 0) {
        proc_str(buf, &pos, len, "charOS 0.19\n");
    } else if (strcmp(path, "tasks") == 0) {
        struct task* t = task_list;
        while (t && pos < len - 32) {
            proc_u32(buf, &pos, len, (uint32_t)t->pid);
            proc_str(buf, &pos, len, " ");
            proc_str(buf, &pos, len, t->name);
            proc_str(buf, &pos, len, " state=");
            proc_u32(buf, &pos, len, (uint32_t)t->state);
            proc_str(buf, &pos, len, " prio=");
            proc_u32(buf, &pos, len, (uint32_t)t->priority);
            proc_str(buf, &pos, len, "\n");
            t = t->next;
        }
    } else {
        return -1;
    }
    buf[pos] = '\0';
    return pos;
}

/* 20B: sanal dosya dağıtıcısı (/proc /sys /dev). path fd'de saklanan
 * tam yoldur; /proc/X -> proc_read(X). offset ile tekrar okumalar da
 * çalışır içeriği her seferinde üretir, dilimlenir. */
int pfs_open(const char* path) {
    if (!path) return -1;
    if (strncmp(path, "/proc/", 6) == 0 || strncmp(path, "/sys/", 5) == 0) {
        const char* sub = path + (path[1] == 'p' ? 6 : 5);
        char dig[24];
        int d = 0;
        while (sub[d] && d < 23) { dig[d] = sub[d]; d++; }
        dig[d] = '\0';
        if (strcmp(dig, "status") == 0 || strcmp(dig, "mem") == 0 ||
            strcmp(dig, "fds") == 0   || strcmp(dig, "uptime") == 0 ||
            strcmp(dig, "version") == 0 || strcmp(dig, "tasks") == 0)
            return 0;
        return -1;
    }
    if (strcmp(path, "/dev/null") == 0 || strcmp(path, "/dev/zero") == 0) return 0;
    return -1;
}

int pfs_read(const char* path, int off, char* buf, int len) {
    if (!path || !buf || len <= 0 || off < 0) return -1;
    if (strncmp(path, "/proc/", 6) == 0) {
        return proc_read(path + 6, buf, len);
    }
    if (strncmp(path, "/sys/", 5) == 0) {
        if (strcmp(path + 5, "version") == 0) {
            const char* v = "charOS 0.19\n";
            int p = 0;
            while (v[p] && p < len - 1) buf[p] = v[p], p++;
            buf[p] = '\0';
            return p;
        }
        return -1;
    }
    if (strcmp(path, "/dev/null") == 0) {
        buf[0] = '\0';
        return 0;
    }
    if (strcmp(path, "/dev/zero") == 0) {
        for (int i = 0; i < len; i++) buf[i] = 0;
        return len;
    }
    return -1;
}

int pfs_write(const char* path, const char* buf, int len) {
    (void)buf;
    if (!path) return -1;
    if (strcmp(path, "/dev/null") == 0) return len;
    return -1;
}

void proc_init(void) {
    vga_puts("[9A] /proc init\n");
    serial_puts("[9A] /proc init\n");
}

int proc_read_pid(const char* path, char* buf, int len) {
    (void)path;
    if (!current_task || len <= 0) return -1;
    char tmp[32];
    int pos = 0;
    // Basit: "pid=X\n" formatı
    strcpy(tmp, "pid=");
    pos = strlen(tmp);
    // PID'yi manuel olarak ekle
    int pid = current_task->pid;
    char rev[12]; int r = 0;
    if (pid == 0) rev[r++] = '0';
    while (pid > 0) { rev[r++] = '0' + (pid % 10); pid /= 10; }
    for (int i = r-1; i >= 0 && pos < len-2; i--) tmp[pos++] = rev[i];
    tmp[pos++] = '\n';
    tmp[pos] = '\0';
    int to_copy = (pos < len) ? pos : len;
    memcpy(buf, tmp, to_copy);
    return to_copy;
}
