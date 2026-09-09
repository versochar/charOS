#include <fs/vfs.h>
#include <fs/chfs.h>
#include <fs/fd.h>
#include <memory/kheap.h>
#include <memory/pmm.h>
#include <string.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <process/task.h>
#include <process/pipe.h>

/* chFS - Char File System (basit ramfs) */

struct inode inode_table[FS_MAX_FILES];
int inode_used[FS_MAX_FILES];
char block_pool[FS_MAX_BLOCKS][FS_BLOCK_SIZE];
int block_used[FS_MAX_BLOCKS];

int chfs_get_size(const char* path) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) return inode_table[i].size;
    }
    return -1;
}
int chfs_read_file(const char* path, char* buf, int max_len) {
    int ino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) if (inode_used[i] && strcmp(inode_table[i].name, path)==0) { ino=i; break; }
    if (ino < 0) return -1;
    int sz = inode_table[ino].size;
    if (sz > max_len) sz = max_len;
    int off = 0, rem = sz;
    while (rem > 0) {
        int blk_idx = off / FS_BLOCK_SIZE;
        int blk_off = off % FS_BLOCK_SIZE;
        if (blk_idx >= CHFS_DATABLOCKS) break;
        int blk = inode_table[ino].blocks[blk_idx];
        if (blk < 0 || blk >= FS_MAX_BLOCKS) break;
        int chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > rem) chunk = rem;
        memcpy(buf+off, &block_pool[blk][blk_off], chunk);
        off += chunk; rem -= chunk;
    }
    return off; /* erken break'te gerçekten okunanı dön */
}

/* 12D: per-task fd_table için helper - current_task üzerinden */
static struct fd_entry* get_fd_table(void) {
    struct task* cur = task_current();
    if (!cur) return 0;
    return cur->fd_table;
}
static struct fd_entry* get_fd(int fd) {
    struct fd_entry* tbl = get_fd_table();
    if (!tbl || fd < 0 || fd >= 16) return 0;
    if (!tbl[fd].valid) return 0;
    return &tbl[fd];
}

/* Basit root inode */
static int root_ino = 0;

void chfs_init(void) {
    memset(inode_table, 0, sizeof(inode_table));
    memset(inode_used, 0, sizeof(inode_used));
    memset(block_pool, 0, sizeof(block_pool));
    memset(block_used, 0, sizeof(block_used));

    // Root inode oluştur (dizin, blok 0)
    inode_table[0].id = 0;
    inode_table[0].type = FT_DIR;
    inode_table[0].size = sizeof(struct dirent); // kök için kendini gösteren bir giriş
    strcpy(inode_table[0].name, "/");
    inode_table[0].permissions = PERM_R | PERM_W | PERM_X;
    inode_table[0].uid = 0; /* 14E */
    inode_table[0].gid = 0;
    inode_table[0].mode = 0755;
    inode_table[0].ref_count = 1;
    inode_used[0] = 1;
    root_ino = 0;

    // Blok 0'ı root için ayır (dirent formatında)
    block_used[0] = 1;
    memset(block_pool[0], 0, FS_BLOCK_SIZE);
    inode_table[0].blocks[0] = 0;
    // Root dizine kendisi için bir dirent yaz
    struct dirent* de = (struct dirent*)block_pool[0];
    strncpy(de->name, "/", 24);
    de->inode_id = 0;

    vga_puts("[7A] chFS init\n");
    serial_puts("[7A] chFS init\n");
}

int fs_create(const char* path) {
    if (!path || strlen(path) == 0 || strlen(path) >= FS_MAX_PATH) return -1;
    /* inode name alanı 32 byte: uzun path'ler sessizce kesilip çakışmasın */
    if (strlen(path) >= sizeof(inode_table[0].name)) return -1;
    for (int i = 1; i < FS_MAX_FILES; i++) {
        if (!inode_used[i]) {
            inode_used[i] = 1;
            inode_table[i].id = i;
            inode_table[i].type = FT_FILE;
            inode_table[i].size = 0;
            strncpy(inode_table[i].name, path, 31);
            inode_table[i].name[31] = '\0';
            for (int k = 0; k < CHFS_DATABLOCKS; k++) inode_table[i].blocks[k] = -1;
            int blk_idx = -1;
            for (int b = 0; b < FS_MAX_BLOCKS; b++) {
                if (!block_used[b]) {
                    block_used[b] = 1;
                    blk_idx = b;
                    memset(block_pool[b], 0, FS_BLOCK_SIZE);
                    break;
                }
            }
            inode_table[i].blocks[0] = blk_idx;
            inode_table[i].permissions = PERM_R | PERM_W | PERM_X;
            inode_table[i].ref_count = 0;
            /* 14E: sahiplik o anki task'tan (yoksa root), mod 0644 */
            {
                struct task* ct = task_current();
                inode_table[i].uid = ct ? ct->uid : 0;
                inode_table[i].gid = ct ? ct->gid : 0;
            }
            inode_table[i].mode = 0644;
            return i;
        }
    }
    return -1;
}

int fs_open(const char* path, int mode) {
    if (!path) return -1;
    /* 20B: sanal dosyalar (/proc /sys /dev) fd-tabanlı okunur */
    if (strncmp(path, "/proc/", 6) == 0 || strncmp(path, "/sys/", 5) == 0 ||
        strncmp(path, "/dev/", 5) == 0) {
        extern int pfs_open(const char* path);
        if (pfs_open(path) != 0) return -1;
        struct fd_entry* tbl = get_fd_table();
        if (!tbl) return -1;
        for (int i = 0; i < 16; i++) {
            if (!tbl[i].valid) {
                tbl[i].valid = 1;
                tbl[i].type = FD_PSEUDO;
                tbl[i].inode_id = -1;
                tbl[i].mode = mode;
                tbl[i].offset = 0;
                tbl[i].pipe = 0;
                int k = 0;
                while (path[k] && k < 31) { tbl[i].pseudo[k] = path[k]; k++; }
                tbl[i].pseudo[k] = '\0';
                return i;
            }
        }
        return -1; // fd dolu
    }
    int ino = -1;
    // Root veya mevcut dosya ara
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i;
            break;
        }
    }
    if (ino < 0) {
        if (mode & O_CREATE) {
            ino = fs_create(path);
        } else {
            return -1; // bulunamadı
        }
    }
    if (ino < 0) return -1;
    /* 14E: mevcut dosyada kip denetimi (dizinler serbest) */
    if (inode_table[ino].type == FT_FILE) {
        int m = mode & 3;
        int need = (m == O_RDONLY) ? PERM_R : (m == O_WRONLY) ? PERM_W :
                   (m == O_RDWR) ? (PERM_R | PERM_W) : 0;
        if (need && fs_can_path(path, need) != 0) return -1;
    }

    // 12D: per-task FD tablosunda boş yer bul
    struct fd_entry* tbl = get_fd_table();
    if (!tbl) return -1;
    /* 19E: TRUNC/APPEND bayrakları (yalnızca dosyalarda) */
    if (inode_table[ino].type == FT_FILE) {
        if ((mode & O_TRUNC) && (mode & 3) != O_RDONLY) {
            for (int b = 0; b < CHFS_DATABLOCKS; b++) {
                int blk = inode_table[ino].blocks[b];
                if (blk >= 0 && blk < FS_MAX_BLOCKS) block_used[blk] = 0;
                inode_table[ino].blocks[b] = -1;
            }
            inode_table[ino].size = 0;
        }
    }
    for (int i = 0; i < 16; i++) {
        if (!tbl[i].valid) {
            tbl[i].valid = 1;
            tbl[i].type = FD_FILE;
            tbl[i].inode_id = ino;
            tbl[i].mode = mode;
            tbl[i].offset = 0;
            tbl[i].pipe = 0;
            /* 19E: APPEND -> ofset dosya sonuna */
            if ((mode & O_APPEND) && inode_table[ino].type == FT_FILE)
                tbl[i].offset = inode_table[ino].size;
            inode_table[ino].ref_count++;
            return i; // fd numarası
        }
    }
    return -1; // fd dolu
}

int fs_read(int fd, char* buf, int len) {
    struct fd_entry* fe = get_fd(fd);
    if (!fe) return -1;
    if (fe->type == FD_PSEUDO) {
        extern int pfs_read(const char* path, int off, char* buf, int len);
        int n = pfs_read(fe->pseudo, fe->offset, buf, len);
        if (n < 0) return -1;
        fe->offset += n;
        return n;
    }
    if (fe->type != FD_FILE) return -1;
    int ino = fe->inode_id;
    if (ino < 0 || ino >= FS_MAX_FILES) return -1;
    int size = inode_table[ino].size;
    int off = fe->offset;
    if (off >= size) return 0; // EOF
    int to_read = len;
    if (off + to_read > size) to_read = size - off;
    // 12C: çok bloklu okuma (8*512)
    int remaining = to_read;
    int out_off = 0;
    while (remaining > 0) {
        int blk_idx = off / FS_BLOCK_SIZE;
        int blk_off = off % FS_BLOCK_SIZE;
        if (blk_idx >= CHFS_DATABLOCKS) break;
        int blk = inode_table[ino].blocks[blk_idx];
        if (blk < 0 || blk >= FS_MAX_BLOCKS) break;
        int chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > remaining) chunk = remaining;
        memcpy(buf + out_off, &block_pool[blk][blk_off], chunk);
        out_off += chunk;
        off += chunk;
        remaining -= chunk;
    }
    fe->offset = off;
    return out_off; /* erken break'te gerçekten okunanı dön */
}

int fs_write(int fd, const char* buf, int len) {
    struct fd_entry* fe = get_fd(fd);
    if (!fe) return -1;
    if (fe->type == FD_PSEUDO) {
        extern int pfs_write(const char* path, const char* buf, int len);
        int n = pfs_write(fe->pseudo, buf, len);
        if (n < 0) return -1;
        fe->offset += n;
        return n;
    }
    if (fe->type != FD_FILE) return -1;
    int ino = fe->inode_id;
    if (ino < 0 || ino >= FS_MAX_FILES) return -1;
    int off = fe->offset;
    int max_size = FS_MAX_BLOCKS * FS_BLOCK_SIZE; // bilgi amaçlı üst sınır
    (void)max_size;
    if (len < 0) return -1;
    int remaining = len;
    int in_off = 0;
    while (remaining > 0) {
        int blk_idx = off / FS_BLOCK_SIZE;
        int blk_off = off % FS_BLOCK_SIZE;
        if (blk_idx >= CHFS_DATABLOCKS) break;
        int blk = inode_table[ino].blocks[blk_idx];
        if (blk < 0 || blk >= FS_MAX_BLOCKS) {
            for (int i = 0; i < FS_MAX_BLOCKS; i++) {
                if (!block_used[i]) {
                    block_used[i] = 1;
                    blk = i;
                    inode_table[ino].blocks[blk_idx] = i;
                    memset(block_pool[i], 0, FS_BLOCK_SIZE);
                    break;
                }
            }
            if (blk < 0 || blk >= FS_MAX_BLOCKS) break;
        }
        int chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > remaining) chunk = remaining;
        memcpy(&block_pool[blk][blk_off], buf + in_off, chunk);
        in_off += chunk;
        off += chunk;
        remaining -= chunk;
        if (off > inode_table[ino].size) inode_table[ino].size = off;
    }
    fe->offset = off;
    {
        int out = len - remaining;
        /* 19A: sıfır-uzunluklu olmayan yazımın 0 dönmesi anomalisidir
         * (blok/inode tükenmesinin erken uyarısı) — sessiz geçme. */
        if (len > 0 && out == 0) {
            int freec = 0;
            for (int bi = 0; bi < FS_MAX_BLOCKS; bi++) if (!block_used[bi]) freec++;
            serial_puts("[chfs] write 0 fd="); serial_puthex(fd);
            serial_puts(" off="); serial_puthex(fe->offset);
            serial_puts(" blk0="); serial_puthex(inode_table[ino].blocks[0]);
            serial_puts(" free="); serial_puthex(freec);
            serial_puts(" mode="); serial_puthex(fe->mode);
            serial_puts(" ino="); serial_puthex(ino);
            serial_puts("\n");
        }
        return out;
    }
}

/* 14C: dup2 — hedefi zorla kapatıp (console pini burada geçersiz)
 * eski girdinin kopyasını yerleştirir. Dönen: newfd / -1. */
int fs_dup2(int oldfd, int newfd) {
    if (oldfd < 0 || oldfd >= 16 || newfd < 0 || newfd >= 16) return -1;
    struct fd_entry* tbl = get_fd_table();
    if (!tbl || !tbl[oldfd].valid) return -1;
    if (oldfd == newfd) return newfd;
    struct fd_entry* dst = &tbl[newfd];
    if (dst->valid) {
        if (dst->type == FD_FILE && dst->inode_id >= 0 &&
            dst->inode_id < FS_MAX_FILES)
            inode_table[dst->inode_id].ref_count--;
        else if (dst->type == FD_PIPE && dst->pipe)
            pipe_close_end(dst->pipe, dst->pipe_write);
        /* console girdisinin ref sayacı yok; üzerine yazılır */
    }
    *dst = tbl[oldfd];
    if (dst->type == FD_FILE && dst->inode_id >= 0 &&
        dst->inode_id < FS_MAX_FILES)
        inode_table[dst->inode_id].ref_count++;
    else if (dst->type == FD_PIPE && dst->pipe) {
        if (dst->pipe_write) dst->pipe->writers++;
        else dst->pipe->readers++;
        dst->pipe->ref_count++;
    }
    return newfd;
}

int fs_close(int fd) {
    struct fd_entry* fe = get_fd(fd);
    if (!fe) return -1;
    // 12D: console fd'ler (0/1/2) kapatılamaz, hep açık kalır
    if (fe->type == FD_CONSOLE) return 0;
    if (fe->type == FD_PIPE) {
        struct pipe* p = fe->pipe;
        int is_w = fe->pipe_write;
        fe->valid = 0;
        fe->type = FD_NONE;
        fe->pipe = 0;
        if (p) pipe_close_end(p, is_w);
        return 0;
    }
    int ino = fe->inode_id;
    fe->valid = 0;
    fe->type = FD_NONE;
    if (ino >= 0 && ino < FS_MAX_FILES) {
        inode_table[ino].ref_count--;
    }
    return 0;
}

/* 19G: belirli bir task'in fd tablosunu kapat (sinyal-ölümü için).
 * fs_close'tan farkı task_current varsaymamasıdır. Console girdileri
 * korunur (fs_close ile aynı kural). Çift-kapatmaya karşı çağıran,
 * ölü task'a tekrar uygulamamalıdır. */
void fs_close_table(struct fd_entry* tbl) {
    if (!tbl) return;
    for (int i = 0; i < 16; i++) {
        if (!tbl[i].valid) continue;
        if (tbl[i].type == FD_CONSOLE) continue;
        if (tbl[i].type == FD_PIPE) {
            struct pipe* p = tbl[i].pipe;
            int is_w = tbl[i].pipe_write;
            tbl[i].valid = 0;
            tbl[i].type = FD_NONE;
            tbl[i].pipe = 0;
            if (p) pipe_close_end(p, is_w);
        } else if (tbl[i].type == FD_FILE) {
            int fino = tbl[i].inode_id;
            tbl[i].valid = 0;
            tbl[i].type = FD_NONE;
            if (fino >= 0 && fino < FS_MAX_FILES) inode_table[fino].ref_count--;
        } else {
            tbl[i].valid = 0;
            tbl[i].type = FD_NONE;
        }
    }
}

int fs_mkdir(const char* path) {
    if (!path || strlen(path) == 0 || strlen(path) >= FS_MAX_PATH) return -1;
    for (int i = 1; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) return -1;
    }
    int ino = fs_create(path);
    if (ino < 0) return -1;
    inode_table[ino].type = FT_DIR;
    inode_table[ino].permissions = PERM_R | PERM_W | PERM_X;
    inode_table[ino].mode = 0755; /* 14E: dizinler gezilebilir */
    // Dizin için blokta dirent yaz (kendisi için '.')
    int blk = inode_table[ino].blocks[0];
    if (blk >= 0 && blk < FS_MAX_BLOCKS) {
        struct dirent* de = (struct dirent*)&block_pool[blk][0];
        memset(de, 0, sizeof(struct dirent));
        strncpy(de->name, ".", 24);
        de->inode_id = ino;
    }
    inode_table[ino].size = sizeof(struct dirent);
    return ino;
}

void fs_list_dir(const char* path) {
    int ino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i; break;
        }
    }
    if (ino < 0) {
        vga_puts("[chFS] dir not found: "); vga_puts(path ? path : "(null)"); vga_puts("\n");
        return;
    }
    if (inode_table[ino].type != FT_DIR) {
        vga_puts("[chFS] not a directory: "); vga_puts(path); vga_puts("\n");
        return;
    }
    int blk = inode_table[ino].blocks[0];
    if (blk < 0 || blk >= FS_MAX_BLOCKS) {
        vga_puts("[chFS] empty dir\n");
        return;
    }
    vga_puts("[chFS] dir listing: "); vga_puts(inode_table[ino].name); vga_puts("\n");
    serial_puts("[chFS] dir: "); serial_puts(inode_table[ino].name); serial_puts("\n");
    // Gerçek dirent formatından oku (512 byte blok içinde)
    int entry_size = sizeof(struct dirent); // ~28 byte
    int max_entries = FS_BLOCK_SIZE / entry_size;
    for (int e = 0; e < max_entries; e++) {
        struct dirent* de = (struct dirent*)&block_pool[blk][e * entry_size];
        if (de->name[0] == 0) break; // boş giriş
        char type_char = '-';
        int de_ino = de->inode_id;
        if (de_ino >= 0 && de_ino < FS_MAX_FILES && inode_used[de_ino]) {
            type_char = (inode_table[de_ino].type == FT_DIR) ? 'd' : '-';
        }
        vga_puts("  "); vga_putc(type_char); vga_putc(' ');
        vga_puts(de->name); vga_puts(" (inode="); vga_putdec(de_ino); vga_puts(")\n");
    }
}

int fs_unlink(const char* path) {
    if (!path || strlen(path) == 0) return -1;
    int ino = -1;
    for (int i = 1; i < FS_MAX_FILES; i++) { // 0 kök, silinemez
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i;
            break;
        }
    }
    if (ino < 0) return -1; // bulunamadı
    for (int k = 0; k < CHFS_DATABLOCKS; k++) {
        int blk = inode_table[ino].blocks[k];
        if (blk >= 0 && blk < FS_MAX_BLOCKS) block_used[blk] = 0;
    }
    inode_used[ino] = 0;
    memset(&inode_table[ino], 0, sizeof(struct inode));
    return 0;
}

void fs_list(void) {
    vga_puts("chFS files:\n");
    serial_puts("chFS files:\n");
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i]) {
            vga_puts("  "); vga_puts(inode_table[i].name);
            vga_puts(" ("); vga_putdec(inode_table[i].size); vga_puts(" bytes)\n");
            serial_puts("  "); serial_puts(inode_table[i].name);
            serial_puts(" size="); serial_puthex(inode_table[i].size); serial_puts("\n");
        }
    }
}

/* 18K: Veri döndüren dizin listeleme (GUI dosya yöneticisi için).
 * Bu ramfs'te dosyalar inode_table'dadır; hiyerarşi basit olduğundan
 * tüm inode'ları kök görünümünde listeler (path "/" ya da alt dizin). */
int chfs_list_dir(const char* path, struct fs_list_entry* out, int max) {
    if (!path || max <= 0) return 0;
    if (!out) return 0;
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES && n < max; i++) {
        if (!inode_used[i]) continue;
        const char* name = inode_table[i].name;
        if (!name || name[0] == 0) continue;
        /* kök dizinin kendisini listeleme */
        if (strcmp(name, "/") == 0) continue;
        /* Basename: son '/'den sonrası (ramfs adları tam yol içerir) */
        const char* base = name;
        for (const char* p = name; *p; p++)
            if (*p == '/') base = p + 1;
        if (!*base) continue;
        out[n].is_dir = (inode_table[i].type == FT_DIR);
        strncpy(out[n].name, base, FS_LIST_NAME_LEN - 1);
        out[n].name[FS_LIST_NAME_LEN - 1] = 0;
        n++;
    }
    return n;
}

/* 14E: izin denetimi. req PERM_R/W/X. root her şeye yetkili. 0 ok.
 * 20C: ek gruplar (task->groups) "other" değil "group" hakkı verir;
 * CAP_DAC_OVERRIDE tüm rwx denetimini atlar. */
int fs_can(int uid, int gid, struct inode* ino, int req) {
    if (!ino) return -1;
    if (uid == 0) return 0;
    int bits;
    if (uid == ino->uid) bits = (ino->mode >> 6) & 7;
    else if (gid == ino->gid) bits = (ino->mode >> 3) & 7;
    else {
        bits = (ino->mode >> 3) & 7;
        struct task* ct = task_current();
        if (ct) {
            int in_grp = 0;
            for (int i = 0; i < TASK_NGROUPS; i++)
                if (ct->groups[i] == ino->gid) { in_grp = 1; break; }
            if (!in_grp) bits = ino->mode & 7;
        } else bits = ino->mode & 7;
    }
    /* PERM bitleri unix rwx ile aynı konumda (4/2/1) */
    return ((bits & req) == req) ? 0 : -1;
}

/* 14E: yola göre o anki task kimliğiyle denetim. Dizinler serbest. */
int fs_can_path(const char* path, int req) {
    if (!path) return -1;
    int ino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i;
            break;
        }
    }
    if (ino < 0) return -1;
    if (inode_table[ino].type == FT_DIR) return 0;
    struct task* ct = task_current();
    int uid = ct ? ct->uid : 0;
    int gid = ct ? ct->gid : 0;
    if (ct && (ct->caps & CAP_DAC_OVERRIDE)) return 0; /* 20C */
    return fs_can(uid, gid, &inode_table[ino], req);
}

/* 14E: mod değiştirme (root veya sahip; 20C: CAP_FOWNER). */
int fs_chmod(const char* path, int mode) {
    if (!path) return -1;
    int ino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i;
            break;
        }
    }
    if (ino < 0) return -1;
    struct task* ct = task_current();
    int uid = ct ? ct->uid : 0;
    if (uid != 0 && uid != inode_table[ino].uid &&
        !(ct && (ct->caps & CAP_FOWNER))) return -1;
    inode_table[ino].mode = mode & 0777;
    return 0;
}

/* 18S: Dosyayı sıfırla (editör kaydetme için, bloklar sahipli kalır) */
int chfs_truncate(const char* path) {
    if (!path) return -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            inode_table[i].size = 0;
            return 0;
        }
    }
    return -1;
}

/* 18S: fd'siz dosya yazma (boot selftestleri + editör için).
 * Dosya yoksa oluşturur, içeriği baştan yazar. Dönüş: yazılan bayt. */
int chfs_write_file(const char* path, const char* buf, int len) {
    if (!path || !buf || len < 0) return -1;
    int ino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i] && strcmp(inode_table[i].name, path) == 0) {
            ino = i;
            break;
        }
    }
    if (ino < 0) {
        ino = fs_create(path);
        if (ino < 0) return -1;
    }
    inode_table[ino].size = 0;
    int off = 0;
    while (off < len) {
        int blk_idx = off / FS_BLOCK_SIZE;
        int blk_off = off % FS_BLOCK_SIZE;
        if (blk_idx >= CHFS_DATABLOCKS) break;
        int blk = inode_table[ino].blocks[blk_idx];
        if (blk < 0 || blk >= FS_MAX_BLOCKS) {
            blk = -1;
            for (int b = 0; b < FS_MAX_BLOCKS; b++) {
                if (!block_used[b]) {
                    block_used[b] = 1;
                    blk = b;
                    inode_table[ino].blocks[blk_idx] = b;
                    memset(block_pool[b], 0, FS_BLOCK_SIZE);
                    break;
                }
            }
            if (blk < 0) break;
        }
        int chunk = FS_BLOCK_SIZE - blk_off;
        if (chunk > len - off) chunk = len - off;
        memcpy(&block_pool[blk][blk_off], buf + off, (uint32_t)chunk);
        off += chunk;
    }
    inode_table[ino].size = off;
    return off;
}

/* 18K: Dosyayı taşı/yeniden adlandır (sürükle-bırak destekli) */
int chfs_move(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) return -1;
    int oino = -1, nino = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (inode_used[i]) {
            if (strcmp(inode_table[i].name, oldpath) == 0) oino = i;
            if (strcmp(inode_table[i].name, newpath) == 0) nino = i;
        }
    }
    if (oino < 0) return -1;         /* kaynak yok */
    if (nino >= 0) return -2;        /* hedef zaten var */
    strncpy(inode_table[oino].name, newpath, 32 - 1);
    inode_table[oino].name[32 - 1] = 0;
    return 0;
}
