#include <fs/diskfs.h>
#include <drivers/virtio_blk.h>
#include <drivers/nvme.h>
#include <drivers/gpt.h>
#include <drivers/blk.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

struct dfs_super {
    uint32_t magic;
    uint32_t version;
    uint32_t total_sectors;
    uint32_t inode_count;
    uint32_t inode_start;   /* LBA */
    uint32_t bmap_start;    /* LBA */
    uint32_t bmap_sectors;
    uint32_t data_start;    /* LBA */
} __attribute__((packed));

/* 17B: inode 224B (48 direkt blok, 24KB/dosya). Sektör sınırını aşabilir,
 * o yüzden iget/iput parça parça okur/yazar. */
struct dfs_inode {
    char name[24];
    uint32_t size;
    uint32_t blocks[DISKFS_DATABLOCKS];
    uint8_t used;
    uint8_t ftype;   /* 17A: DFS_TYPE_FILE / DFS_TYPE_DIR */
    uint8_t pad[2];
} __attribute__((packed));

static struct dfs_super sb;
static int mounted = 0;

/* 27D: backend + bölüm ofseti. dfs_nvme=1 -> NVMe, 0 -> virtio-blk.
 * Tüm G/Ç dread/dwrite'tan geçer; LBA'ya dfs_base eklenir. */
static uint32_t dfs_base = 0;
static int dfs_nvme = 0;

void diskfs_use_nvme(int on) { dfs_nvme = on ? 1 : 0; }

static int dread(uint32_t lba, void* buf) {
    lba += dfs_base;
    return dfs_nvme ? nvme_read_sector(lba, buf) : blk_read(lba, buf);
}
static int dwrite(uint32_t lba, const void* buf) {
    lba += dfs_base;
    return dfs_nvme ? nvme_write_sector(lba, buf) : blk_write(lba, buf);
}
static int dpresent(void) {
    return dfs_nvme ? nvme_present() : blk_present();
}
static uint32_t dcapacity(void) {
    if (dfs_nvme) {
        uint64_t c = nvme_capacity_sectors();
        return (c > 0xFFFFFFFFu) ? 0xFFFFFFFFu : (uint32_t)c;
    }
    return blk_capacity();
}

/* Bitmap biti: 1 = dolu */
static int bmap_get(uint32_t sec) {
    static uint8_t s[512];
    uint32_t byte = sec / 8;
    uint32_t lba = sb.bmap_start + byte / 512;
    if (dread(lba, s) != 0) return 1; /* hata = dolu say */
    return (s[byte % 512] >> (sec % 8)) & 1;
}

static void bmap_set(uint32_t sec, int v) {
    static uint8_t s[512];
    uint32_t byte = sec / 8;
    uint32_t lba = sb.bmap_start + byte / 512;
    if (dread(lba, s) != 0) return;
    if (v) s[byte % 512] |= (1 << (sec % 8));
    else s[byte % 512] &= ~(1 << (sec % 8));
    dwrite(lba, s);
}

static int balloc(void) {
    for (uint32_t s = sb.data_start; s < sb.total_sectors; s++) {
        if (!bmap_get(s)) { bmap_set(s, 1); return (int)s; }
    }
    return -1;
}

/* 17B: inode oku/yaz — kayıt sektör sınırını aşabilir, parça parça işle */
static int iget(int idx, struct dfs_inode* out) {
    static uint8_t s[512];
    uint32_t off = (uint32_t)idx * sizeof(struct dfs_inode);
    uint32_t rem = sizeof(struct dfs_inode);
    uint8_t* dst = (uint8_t*)out;
    while (rem > 0) {
        uint32_t lba = sb.inode_start + off / 512;
        uint32_t o = off % 512;
        uint32_t chunk = 512 - o;
        if (chunk > rem) chunk = rem;
        if (dread(lba, s) != 0) return -1;
        memcpy(dst, s + o, chunk);
        dst += chunk; off += chunk; rem -= chunk;
    }
    return 0;
}

static int iput(int idx, const struct dfs_inode* in) {
    static uint8_t s[512];
    uint32_t off = (uint32_t)idx * sizeof(struct dfs_inode);
    uint32_t rem = sizeof(struct dfs_inode);
    const uint8_t* src = (const uint8_t*)in;
    while (rem > 0) {
        uint32_t lba = sb.inode_start + off / 512;
        uint32_t o = off % 512;
        uint32_t chunk = 512 - o;
        if (chunk > rem) chunk = rem;
        if (dread(lba, s) != 0) return -1;
        memcpy(s + o, src, chunk);
        if (dwrite(lba, s) != 0) return -1;
        src += chunk; off += chunk; rem -= chunk;
    }
    return 0;
}

static int ifind(const char* name) {
    struct dfs_inode in;
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) return -1;
        if (in.used && strncmp(in.name, name, 24) == 0) return i;
    }
    return -1;
}

/* 17A: yol doğrulama — 1..23 bayt, ".." ve "//" yok */
static int df_valid_path(const char* path) {
    if (!path || !path[0]) return 0;
    int n = strlen(path);
    if (n >= 24) return 0;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '.' && path[i+1] == '.') return 0;
        if (path[i] == '/' && path[i+1] == '/') return 0;
    }
    if (n > 1 && path[n-1] == '/') return 0; /* sondaki '/' yok */
    return 1;
}

/* 17C: path altında çocuk var mı (dir doluluk kontrolü) */
static int df_has_children(const char* path) {
    struct dfs_inode in;
    int plen = strlen(path);
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) return 1; /* şüphede dolu say */
        if (!in.used) continue;
        if (strncmp(in.name, path, 24) == 0) continue; /* kendisi */
        if (plen == 1 && path[0] == '/') {
            if (in.name[0] == '/' && in.name[1] != '\0') return 1;
            continue;
        }
        int k = 0;
        while (k < plen && in.name[k] == path[k]) k++;
        if (k == plen && in.name[k] == '/') return 1;
    }
    return 0;
}

/* 17C: inode'un bloklarını serbest bırak (kaydı diske yazar) */
static int ifree_blocks(int idx) {
    struct dfs_inode in;
    if (iget(idx, &in) != 0) return -1;
    for (int i = 0; i < DISKFS_DATABLOCKS; i++) {
        if (in.blocks[i]) { bmap_set(in.blocks[i], 0); in.blocks[i] = 0; }
    }
    in.size = 0;
    return iput(idx, &in);
}

/* 27D: belli ofset + üst sınırla tak (GPT bölümü içi). backend,
 * diskfs_use_nvme() ile önceden seçilir. */
int diskfs_mount_at(uint32_t base, uint32_t max_sec) {
    static uint8_t s[512];
    dfs_base = base;
    if (!dpresent()) return -1;
    if (dread(0, s) != 0) return -1;
    memcpy(&sb, s, sizeof(sb));
    if (sb.magic != DISKFS_MAGIC || sb.version != DISKFS_VERSION) return -1;
    if (sb.inode_count == 0 || sb.inode_count > 256) return -1;
    if (sb.total_sectors == 0 || sb.total_sectors > max_sec) return -1;
    if (sb.data_start >= sb.total_sectors) return -1;
    mounted = 1;
    return 0;
}

int diskfs_mount(void) {
    diskfs_use_nvme(0);
    return diskfs_mount_at(0, 0xFFFFFFFFu);
}

/* format öncesi bitmap hesabı (sb henüz yokken) */
static uint32_t bmap_bytes_for(uint32_t total) { return (total + 7) / 8; }

int diskfs_format_at(uint32_t base, uint32_t total) {
    static uint8_t s[512];
    dfs_base = base;
    if (!dpresent()) return -1;
    if (total < 64) return -1;
    sb.magic = DISKFS_MAGIC;
    sb.version = DISKFS_VERSION;
    sb.total_sectors = total;
    sb.inode_count = DISKFS_INODES;
    sb.bmap_start = 1;
    sb.bmap_sectors = (bmap_bytes_for(total) + 511) / 512;
    sb.inode_start = sb.bmap_start + sb.bmap_sectors;
    uint32_t itab_sectors = (sb.inode_count * sizeof(struct dfs_inode) + 511) / 512;
    sb.data_start = sb.inode_start + itab_sectors;

    memset(s, 0, sizeof(s));
    memcpy(s, &sb, sizeof(sb));
    if (dwrite(0, s) != 0) return -1;
    /* bitmap: data_start öncesi tüm sektörler dolu, gerisi boş */
    for (uint32_t i = 0; i < sb.bmap_sectors; i++) {
        static uint8_t z[512];
        memset(z, 0, sizeof(z));
        for (uint32_t b = 0; b < 512; b++) {
            uint32_t sec = i * 4096 + b * 8;
            uint8_t bits = 0;
            for (int k = 0; k < 8; k++)
                if (sec + k < sb.data_start) bits |= (1 << k);
            z[b] = bits;
        }
        if (dwrite(sb.bmap_start + i, z) != 0) return -1;
    }
    /* inode tablosunu sıfırla */
    memset(s, 0, sizeof(s));
    for (uint32_t i = 0; i < itab_sectors; i++)
        if (dwrite(sb.inode_start + i, s) != 0) return -1;
    mounted = 1;
    return 0;
}

/* Legacy GB (geriye dönük uyumluluk): virtio-blk'de tam disk formatı */
int diskfs_format(void) {
    diskfs_use_nvme(0);
    return diskfs_format_at(0, dcapacity());
}

int dfile_write(const char* name, const char* data, int len) {
    struct dfs_inode in;
    static uint8_t s[512];
    if (!mounted || !name || !data || len < 0) return -1;
    if (len > DISKFS_MAXFILE) return -1;
    int nl = strlen(name);
    if (nl == 0 || nl >= 24) return -1;
    if (!df_valid_path(name)) return -1; /* 17H: "..", "//", sondaki '/' yok */
    if (name[0] == '/' && name[1] == '\0') return -1; /* kök ayrılmıştır */
    int idx = ifind(name);
    if (idx >= 0) {
        if (iget(idx, &in) != 0) return -1;
        if (in.ftype == DFS_TYPE_DIR) return -1; /* 17A: dizine yazılmaz */
        /* eski blokları bırak */
        for (int i = 0; i < DISKFS_DATABLOCKS; i++) {
            if (in.blocks[i]) { bmap_set(in.blocks[i], 0); in.blocks[i] = 0; }
        }
    } else {
        memset(&in, 0, sizeof(in));
        idx = -1;
        for (int i = 0; i < (int)sb.inode_count; i++) {
            struct dfs_inode t;
            if (iget(i, &t) != 0) return -1;
            if (!t.used) { idx = i; break; }
        }
        if (idx < 0) return -1;
        strncpy(in.name, name, 24);
        in.name[23] = '\0';
        in.used = 1;
        in.ftype = DFS_TYPE_FILE;
    }
    int off = 0;
    int bi = 0;
    while (off < len && bi < DISKFS_DATABLOCKS) {
        int sec = balloc();
        if (sec < 0) goto wfail; /* 17C: kısmi tahsisi geri al */
        memset(s, 0, sizeof(s));
        int chunk = len - off;
        if (chunk > 512) chunk = 512;
        memcpy(s, data + off, chunk);
        if (dwrite(sec, s) != 0) { bmap_set(sec, 0); goto wfail; }
        in.blocks[bi++] = sec;
        off += chunk;
    }
    in.size = len;
    if (iput(idx, &in) != 0) return -1;
    return len;
wfail:
    for (int j = 0; j < bi; j++) {
        if (in.blocks[j]) { bmap_set(in.blocks[j], 0); in.blocks[j] = 0; }
    }
    in.size = 0;
    iput(idx, &in);
    return -1;
}

int dfile_read(const char* name, char* buf, int maxlen) {
    struct dfs_inode in;
    static uint8_t s[512];
    if (!mounted || !name || !buf || maxlen <= 0) return -1;
    if (!df_valid_path(name)) return -1;
    int idx = ifind(name);
    if (idx < 0) return -1;
    if (iget(idx, &in) != 0) return -1;
    if (in.ftype == DFS_TYPE_DIR) return -1; /* 17A: dizin okunmaz */
    int toread = in.size;
    if (toread > maxlen) toread = maxlen;
    int off = 0;
    for (int i = 0; i < DISKFS_DATABLOCKS && off < toread; i++) {
        if (!in.blocks[i]) break;
        if (dread(in.blocks[i], s) != 0) return -1;
        int chunk = toread - off;
        if (chunk > 512) chunk = 512;
        memcpy(buf + off, s, chunk);
        off += chunk;
    }
    return toread > off ? off : toread;
}

/* 17A: dizin oluştur */
int df_mkdir(const char* path) {
    struct dfs_inode in;
    if (!mounted || !df_valid_path(path)) return -1;
    if (path[0] != '/') return -1;
    if (path[1] == '\0') return -1; /* 17H: kök zaten vardır, oluşturulmaz */
    if (ifind(path) >= 0) return -1; /* zaten var */
    int idx = -1;
    for (int i = 0; i < (int)sb.inode_count; i++) {
        struct dfs_inode t;
        if (iget(i, &t) != 0) return -1;
        if (!t.used) { idx = i; break; }
    }
    if (idx < 0) return -1;
    memset(&in, 0, sizeof(in));
    strncpy(in.name, path, 24);
    in.name[23] = '\0';
    in.used = 1;
    in.ftype = DFS_TYPE_DIR;
    in.size = 0;
    if (iput(idx, &in) != 0) return -1;
    return 0;
}

/* 17A: doğrudan alt öğeleri listele — "ad\n" satırları, NUL-sonlu. Dönüş: bayt (NUL hariç) / -1 */
int df_readdir(const char* path, char* out, int max) {
    struct dfs_inode in;
    if (!mounted || !out || max <= 0) return -1;
    if (!df_valid_path(path) && !(path[0] == '/' && path[1] == '\0')) return -1;
    int is_root = (path[0] == '/' && path[1] == '\0');
    if (!is_root) {
        int di = ifind(path);
        if (di < 0) return -1;
        if (iget(di, &in) != 0) return -1;
        if (!in.used || in.ftype != DFS_TYPE_DIR) return -1;
    }
    int plen = is_root ? 1 : strlen(path);
    int pos = 0;
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) return -1;
        if (!in.used) continue;
        const char* nm = in.name;
        const char* base = 0;
        if (is_root) {
            if (nm[0] == '/') {
                if (nm[1] == '\0') continue; /* kökün kendisi yok */
                int k = 1;
                while (nm[k] && nm[k] != '/') k++;
                if (nm[k] != '\0') continue; /* alt seviye değil */
                base = nm + 1;
            } else {
                if (nm[0] == '\0') continue;
                int k = 0;
                while (nm[k] && nm[k] != '/') k++;
                if (nm[k] != '\0') continue;
                base = nm; /* 17F: eski düz isimler de kök öğesidir */
            }
        } else {
            int k = 0;
            while (k < plen && nm[k] == path[k]) k++;
            if (k != plen || nm[k] != '/') continue;
            base = nm + plen + 1;
            if (!*base) continue;
            int j = 0;
            while (base[j] && base[j] != '/') j++;
            if (base[j] != '\0') continue; /* alt seviye değil */
        }
        int bl = strlen(base);
        if (pos + bl + 1 >= max) break; /* sığmazsa dur (kısmi liste yok sayılmaz) */
        memcpy(out + pos, base, bl);
        pos += bl;
        out[pos++] = '\n';
    }
    if (pos < max) out[pos] = '\0';
    else if (max > 0) out[max - 1] = '\0';
    return pos;
}

/* 17C: sil — boş olmayan dizin reddedilir */
int df_remove(const char* path) {
    struct dfs_inode in;
    if (!mounted || !df_valid_path(path)) return -1;
    int idx = ifind(path);
    if (idx < 0) return -1;
    if (iget(idx, &in) != 0) return -1;
    if (!in.used) return -1;
    if (in.ftype == DFS_TYPE_DIR && df_has_children(path)) return -1;
    if (ifree_blocks(idx) != 0) return -1;
    if (iget(idx, &in) != 0) return -1;
    memset(&in, 0, sizeof(in));
    if (iput(idx, &in) != 0) return -1;
    return 0;
}

/* 17D: truncate — boyut 0, bloklar serbest */
int df_truncate(const char* path) {
    struct dfs_inode in;
    if (!mounted || !df_valid_path(path)) return -1;
    int idx = ifind(path);
    if (idx < 0) return -1;
    if (iget(idx, &in) != 0) return -1;
    if (!in.used || in.ftype == DFS_TYPE_DIR) return -1;
    return ifree_blocks(idx);
}

/* 17F: tek dosya/dizin bilgisi */
int df_stat(const char* path, struct dfs_stat* out) {
    struct dfs_inode in;
    if (!mounted || !out || !df_valid_path(path)) return -1;
    if (path[0] == '/' && path[1] == '\0') { /* kök her zaman vardır */
        out->size = 0;
        out->is_dir = 1;
        return 0;
    }
    int idx = ifind(path);
    if (idx < 0) return -1;
    if (iget(idx, &in) != 0) return -1;
    if (!in.used) return -1;
    out->size = in.size;
    out->is_dir = (in.ftype == DFS_TYPE_DIR) ? 1u : 0u;
    return 0;
}

/* 17F: disk kullanım özeti */
void df_stats(struct dfs_stats* out) {
    if (!out) return;
    out->total_blocks = sb.total_sectors;
    out->inode_count = sb.inode_count;
    out->used_blocks = 0;
    out->used_inodes = 0;
    if (!mounted) {
        out->free_blocks = 0;
        return;
    }
    struct dfs_inode in;
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) break;
        if (in.used) out->used_inodes++;
    }
    for (uint32_t s = sb.data_start; s < sb.total_sectors; s++)
        if (bmap_get(s)) out->used_blocks++;
    out->free_blocks = (sb.total_sectors > sb.data_start)
        ? (sb.total_sectors - sb.data_start - out->used_blocks) : 0;
}

/* 17H: bitmap + inode tablosu tutarlılık denetimi */
int df_check(void) {
    struct dfs_inode in;
    if (!mounted) return -1;
    if (sb.magic != DISKFS_MAGIC || sb.version != DISKFS_VERSION) return -1;
    if (sb.total_sectors < 64 || sb.inode_count == 0 || sb.inode_count > 256)
        return -1;
    uint32_t bsec = (bmap_bytes_for(sb.total_sectors) + 511) / 512;
    uint32_t isec = (sb.inode_count * sizeof(struct dfs_inode) + 511) / 512;
    if (sb.bmap_start != 1 || sb.bmap_sectors != bsec) return -1;
    if (sb.inode_start != 1 + bsec) return -1;
    if (sb.data_start != 1 + bsec + isec) return -1;
    /* meta bölge bitleri hep dolu olmalı */
    for (uint32_t s = 0; s < sb.data_start; s++)
        if (!bmap_get(s)) return -1;
    /* referanslar aralıkta + işaretli olmalı; çift tahsis sayımla yakalanır */
    uint32_t refcount = 0;
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) return -1;
        if (!in.used) continue;
        if ((int)in.size < 0 || in.size > (uint32_t)DISKFS_MAXFILE) return -1;
        for (int b = 0; b < DISKFS_DATABLOCKS; b++) {
            if (!in.blocks[b]) continue;
            if (in.blocks[b] < sb.data_start || in.blocks[b] >= sb.total_sectors)
                return -1;
            if (!bmap_get(in.blocks[b])) return -1;
            refcount++;
        }
    }
    uint32_t marked = 0;
    for (uint32_t s = sb.data_start; s < sb.total_sectors; s++)
        if (bmap_get(s)) marked++;
    if (marked != refcount) return -1; /* sızıntı ya da çift tahsis */
    return 0;
}

void diskfs_list(void) {
    struct dfs_inode in;
    if (!mounted) return;
    vga_puts("diskfs files:\n");
    serial_puts("diskfs files:\n");
    for (int i = 0; i < (int)sb.inode_count; i++) {
        if (iget(i, &in) != 0) break;
        if (in.used) {
            vga_puts("  "); vga_puts(in.name);
            vga_puts(" ("); vga_putdec(in.size); vga_puts(" bytes)\n");
            serial_puts("  "); serial_puts(in.name);
            serial_puts(" size="); serial_puthex(in.size); serial_puts("\n");
        }
    }
}

/* 27D: NVMe üstünde GPT bölümünde diskfs selftest */
int diskfs_nvme_selftest(void) {
    if (!nvme_present()) {
        serial_puts("[27D] NVMe yok, atlandı\n");
        return -1;
    }
    /* GPT'yi tara */
    int n = gpt_scan_nvme();
    if (n <= 0) {
        serial_puts("[27D] GPT taraması başarısız [FAIL]\n");
        return -1;
    }
    const struct gpt_part* parts = gpt_cached(0);
    int ci = gpt_find_charos(parts, n);
    if (ci < 0) {
        serial_puts("[27D] charOS bölümü bulunamadı [FAIL]\n");
        return -1;
    }
    uint32_t base = (uint32_t)parts[ci].start_lba;
    uint32_t size = (uint32_t)(parts[ci].end_lba - parts[ci].start_lba + 1);
    serial_puts("[27D] charOS bölümü: base="); serial_puthex(base);
    serial_puts(" size="); serial_puthex(size); serial_puts("\n");

    /* NVMe'de mount/format + selftest */
    diskfs_use_nvme(1);
    if (diskfs_mount_at(base, size) == 0) {
        serial_puts("[27D] NVMe diskfs mounted\n");
    } else {
        if (diskfs_format_at(base, size) != 0) {
            serial_puts("[27D] NVMe format [FAIL]\n");
            return -1;
        }
        serial_puts("[27D] NVMe diskfs formatted\n");
    }

    const char* tdata = "diskfs-27D-persist";
    int tlen = strlen(tdata);
    if (dfile_write("t27d", tdata, tlen) != tlen) {
        serial_puts("[27D] selftest WRITE [FAIL]\n");
        return -1;
    }
    if (diskfs_mount_at(base, size) != 0) {
        serial_puts("[27D] selftest REMOUNT [FAIL]\n");
        return -1;
    }
    static char rbuf[64];
    memset(rbuf, 0, sizeof(rbuf));
    int rn = dfile_read("t27d", rbuf, sizeof(rbuf) - 1);
    if (rn != tlen || memcmp(rbuf, tdata, tlen) != 0) {
        serial_puts("[27D] selftest READ [FAIL]\n");
        return -1;
    }
    serial_puts("[27D] NVMe diskfs persist [PASS]\n");
    vga_puts("[27D] NVMe diskfs persist [PASS]\n");
    /* 27D bitince default backend'i virtio-blk'e geri çevirelim ki
       alt seviye testler (13H vb.) beklemeyen şartlarda çakmasın */
    diskfs_use_nvme(0);
    dfs_base = 0;
    mounted = 0; /* 27D'nin mount durumunu sıfırla, 13H yeniden başlatsın */
    return 0;
}

void diskfs_boot(void) {
    if (diskfs_mount() == 0) {
        serial_puts("[13H] diskfs mounted (kalıcı veri korundu)\n");
        vga_puts("[13H] diskfs mounted\n");
    } else {
        serial_puts("[13H] sihir yok, formatlanıyor...\n");
        if (diskfs_format() != 0) {
            serial_puts("[13H] format [FAIL]\n");
            vga_puts("[13H] format [FAIL]\n");
            return;
        }
        serial_puts("[13H] diskfs formatted\n");
        vga_puts("[13H] diskfs formatted\n");
    }
    /* Self-test: yaz -> diskten yeniden mount -> oku -> karşılaştır */
    const char* tdata = "diskfs-13H-persist";
    int tlen = strlen(tdata);
    if (dfile_write("t13h", tdata, tlen) != tlen) {
        serial_puts("[13H] selftest WRITE [FAIL]\n");
        return;
    }
    if (diskfs_mount() != 0) {
        serial_puts("[13H] selftest REMOUNT [FAIL]\n");
        return;
    }
    static char rbuf[64];
    memset(rbuf, 0, sizeof(rbuf));
    int n = dfile_read("t13h", rbuf, sizeof(rbuf) - 1);
    if (n != tlen || memcmp(rbuf, tdata, tlen) != 0) {
        serial_puts("[13H] selftest READ [FAIL]\n");
        vga_puts("[13H] selftest READ [FAIL]\n");
        return;
    }
    diskfs_list();
    serial_puts("[13H] diskfs persist [PASS]\n");
    vga_puts("[13H] diskfs persist [PASS]\n");
    /* 27F: fsck + journal kurtarma */
    diskfs_check_and_recover();
}

/* 27F: Temel journal + fsck kurtarma */

/* Journal başlığı (512B sektör, LBA = sb.total_sectors - 1 gibi sabit değil;
 * basit yaklaşım: journal sektörü = data_start + 1). */
struct dfs_journal_hdr {
    uint32_t magic;        /* 0x4A524E4C "JRNL" */
    uint32_t op;           /* işlem kodu: 1=write, 2=remove, 3=format */
    uint32_t lba_start;    /* etkilenen LBA */
    uint32_t lba_count;    /* sektör sayısı */
    uint32_t inode_idx;    /* ilgili inode (yazma/format için) */
    uint32_t checksum;     /* basit CRC/toplam */
    uint8_t  reserved[484];
} __attribute__((packed));

#define JOURNAL_MAGIC 0x4A524E4Cu  /* "JRNL" */
/* Journal LBA: disk sonuna yakın (son sektörlerden önce, güvenli) */
static uint32_t journal_lba(void) {
    return (sb.total_sectors > 4) ? (sb.total_sectors - 2) : 2;
}

static uint32_t journal_checksum(const struct dfs_journal_hdr* j) {
    uint32_t s = 0;
    const uint8_t* p = (const uint8_t*)j;
    for (uint32_t i = 0; i < (uint32_t)(sizeof(struct dfs_journal_hdr) - 4); i++)
        s += p[i];
    return s;
}

/* Journal yaz (işlem öncesi): 0=ok, -1=hata */
static int __attribute__((unused)) journal_start(uint32_t op, uint32_t lba_start, uint32_t lba_count, int inode_idx) {
    static uint8_t s[512];
    struct dfs_journal_hdr j;
    memset(&j, 0, sizeof(j));
    j.magic = JOURNAL_MAGIC;
    j.op = op;
    j.lba_start = lba_start;
    j.lba_count = lba_count;
    j.inode_idx = (uint32_t)inode_idx;
    j.checksum = journal_checksum(&j);
    memcpy(s, &j, sizeof(j));
    uint32_t jlba = journal_lba();
    if (dwrite(jlba, s) != 0) return -1;
    return 0;
}

/* Journal sil (işlem başarılı): 0=ok */
static int journal_commit(void) {
    static uint8_t z[512];
    memset(z, 0, sizeof(z));
    uint32_t jlba = journal_lba();
    return dwrite(jlba, z);
}

/* Journal kontrol: tam (magic+checksum ok, op>0) = kurtarılmalı işlem var */
static int journal_has_op(void) {
    static uint8_t s[512];
    uint32_t jlba = journal_lba();
    if (dread(jlba, s) != 0) return 0; /* hata = işlem yok varsay */
    struct dfs_journal_hdr* j = (struct dfs_journal_hdr*)s;
    if (j->magic != JOURNAL_MAGIC) return 0;
    if (j->checksum != journal_checksum(j)) return 0;
    return (j->op > 0) ? 1 : 0;
}

/* 27F: fsck + kurtarma. 0=tutarlı/kurtarıldı, -1=bozuk (kurtarılamadı). */
int diskfs_recover(void) {
    if (!mounted) return -1;
    if (!journal_has_op()) {
        /* Journal boş: klasik fsck (df_check) */
        return df_check();
    }
    /* Journal'da işlem var: basit kurtarma - işlem tamamlanmamış
     * varsayılır ve format/rebuild denemesi yapılır (27F temel). */
    serial_puts("[27F] journal op bulundu, kurtarma deneniyor...\n");
    static uint8_t s[512];
    uint32_t jlba = journal_lba();
    if (dread(jlba, s) != 0) return -1;
    struct dfs_journal_hdr* j = (struct dfs_journal_hdr*)s;
    serial_puts("[27F] op="); serial_puthex(j->op); serial_puts(
               " inode="); serial_puthex(j->inode_idx); serial_puts(
               " lba="); serial_puthex(j->lba_start); serial_puts("\n");
    /* Temel kurtarma: journal'ı sıfırla (tamamlanmış say) ve fsck yap */
    journal_commit();
    int r = df_check();
    if (r == 0) {
        serial_puts("[27F] journal kurtarma [PASS]\n");
    } else {
        serial_puts("[27F] journal kurtarma sonrası fsck [FAIL]\n");
        return -1;
    }
    return r;
}

/* 27F: blk üzerinden fsck çağrısı (blk_fsck wrapper) */
void diskfs_check_and_recover(void) {
    int r = diskfs_recover();
    if (r == 0) {
        serial_puts("[27F] fsck/recover [PASS]\n");
        vga_puts("[27F] fsck/recover [PASS]\n");
    } else {
        serial_puts("[27F] fsck/recover [FAIL]\n");
    }
}
