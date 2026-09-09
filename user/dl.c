/* 15A-15H: mini dinamik yükleyici. chfs dosyasını okur, PT_LOAD'ları
 * anonim mmap'e yerleştirir, sysv-hash ile çözer, REL/JMPREL uygular.
 * 15C: per-thread TLS (___tls_get_addr + DTPMOD32/GD), 15D: program
 * exports + module'lar-arası interposition + COPY, 15E: refcount/dedupe,
 * 15F: sınır denetimleri + hata geri alımı, 15G: init/fini dizileri,
 * 15H: ET_DYN (PIE) giriş noktası (dl_entry). */
#include "dl.h"

#define SYS_OPEN 10
#define SYS_CLOSE 11
#define SYS_READ 2
#define SYS_WRITE 1
#define SYS_MMAP 90
#define SYS_MUNMAP 91
#define SYS_TLS_SET 148
#define SYS_TLS_GET 149
#define O_RDONLY 1
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

#define DL_MAX 4
#define DL_FILE_MAX (64 * 1024)
#define DL_COPY_MAX 8
#define DL_NEED_MAX 8
#define MAIN_BASE   0x1000000   /* userprog bağlama adresi (16H) */

static unsigned dl_sys(unsigned n, unsigned a, unsigned b, unsigned c) {
    unsigned ret;
    asm volatile("int $0x80"
        : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}

static int dl_strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void* dl_memcpy(void* d, const void* s, unsigned n) {
    char* dd = (char*)d;
    const char* ss = (const char*)s;
    for (unsigned i = 0; i < n; i++) dd[i] = ss[i];
    return d;
}

static void dl_memset(void* d, int c, unsigned n) {
    char* p = (char*)d;
    for (unsigned i = 0; i < n; i++) p[i] = (char)c;
}

/* ELF32 (lite) */
#define ET_DYN 3
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_TLS 7
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_STRSZ 10
#define DT_INIT 12
#define DT_FINI 13
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTRELSZ 2
#define DT_JMPREL 23
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_TLS_GD 16
#define R_386_TLS_DTPMOD32 35
#define R_386_TLS_DTPOFF32 36
#define STB_WEAK 2
#define ELFCLASS32 1
#define ELFDATA2LSB 1

struct ehdr {
    unsigned char e_ident[16];
    unsigned short e_type, e_machine;
    unsigned ver;
    unsigned entry, phoff, shoff;
    unsigned eflags;
    unsigned short ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};

struct phdr {
    unsigned type, offset, vaddr, paddr, filesz, memsz, flags, align;
};

struct dyn { int tag; unsigned val; };
struct sym {
    unsigned name, value, size;
    unsigned char info, other;
    unsigned short shndx;
};
struct rel { unsigned off; unsigned info; };

struct dl_handle {
    int used;
    unsigned refs;          /* 15E: dedupe sayacı */
    char name[24];          /* 15E: dedupe anahtarı */
    unsigned base;          /* yükleme tabanı (min vaddr'a göre) */
    unsigned lo;            /* haritalı aralık alt sınırı (vaddr) */
    unsigned map;           /* mmap adresi */
    unsigned map_len;
    unsigned entry;         /* 15H: ELF giriş noktası (vaddr) */
    unsigned* buckets;
    unsigned nbucket;
    unsigned* chains;
    unsigned nchain;
    struct sym* symtab;
    const char* strtab;
    unsigned strsz;
    /* 15C: TLS */
    unsigned tls_memsz;
    unsigned tls_filesz;
    unsigned tls_align;
    unsigned tls_off;       /* thread TLS bloğundaki hizalanmış ofset */
    const void* tls_init;   /* .tdata ilk-veri kopyası (mmap) / 0 */
    /* 15G: init/fini */
    unsigned dt_init, dt_fini;
    unsigned init_arr, init_n, fini_arr, fini_n;
    /* 16C: yükleme sırası (RTLD_NEXT/DEFAULT kapsamı) */
    int seq;
    /* 16D: lazy PLT */
    unsigned jmprel, jmprel_n;  /* .rel.plt bölge vaddr + adet */
    unsigned lazy_map;          /* trampolin sayfası (0=yok) */
    unsigned lazy_use;
    /* 16E: DT_NEEDED bağımlılıkları */
    unsigned need[DL_NEED_MAX];
    unsigned need_n;
    /* 16F: RTLD_GLOBAL görünürlük */
    char global;
    /* 16G: phdr kopyası */
    unsigned phnum;
    struct phdr phdrs[16];
};

static struct dl_handle handles[DL_MAX];
static int dl_seq;              /* 16C: artan yükleme sırası sayaç */

/* 16A: en son hata string'i (pozitif işlemde temizlenir) */
static const char* dl_err;
static void dl_seterr(const char* e) { dl_err = e; }
const char* dlerror(void) {
    const char* e = dl_err;
    dl_err = 0;
    return e;
}

/* 16H: ana program pseudo-tutamacı */
static struct dl_handle main_ph;
static int dl_main_ph_init(void) {
    if (main_ph.used) return 0;
    dl_memset(&main_ph, 0, sizeof(main_ph));
    main_ph.used = 1;
    main_ph.refs = 1;
    main_ph.seq = 0x7FFFFFFF;
    main_ph.name[0] = 'm'; main_ph.name[1] = 'a'; main_ph.name[2] = 'i';
    main_ph.name[3] = 'n'; main_ph.name[4] = 0;
    return 0;
}

/* 16D: lazy trampolin dağıtıcısı (trambolinler buraya zıplar)
 * Strateji: küçük bir çıplak-asm girişi GOT slot'unu ve hedefi global
 * değişkenlere yazdıran C yardımcısını çağırır, sonra hedefe atlar.
 * Ayarlama kendini: GOT artık hedefi gösterir; tekrar çağrıda doğrudan gider. */
static unsigned dl_lazy_saddr __attribute__((used));  /* çözülecek GOT konumu */
static unsigned dl_lazy_taddr __attribute__((used));  /* çözülen hedef adresi */

static unsigned* dl_ptr(struct dl_handle* h, unsigned vaddr, unsigned need);
static int dl_sym_by_idx(struct dl_handle* h, unsigned idx, unsigned* out);

static void dl_lazy_resolve(unsigned ctx) __attribute__((used));
static void dl_lazy_resolve(unsigned ctx) {
    unsigned k = ctx & 0xFFu;
    unsigned hidx = (ctx >> 8) & 0xFFu;
    struct dl_handle* h = &handles[hidx];
    unsigned s = 0;
    struct rel* r = (struct rel*)dl_ptr(h, h->jmprel + k * 8, 8);
    if (!r) for (;;) asm volatile("hlt");
    if (dl_sym_by_idx(h, r->info >> 8, &s) != 0) for (;;) asm volatile("hlt");
    dl_lazy_saddr = (unsigned)dl_ptr(h, r->off, 4);
    dl_lazy_taddr = s;
}

static void dl_lazy_dispatch(void) __attribute__((naked, noreturn));
static void dl_lazy_dispatch(void) {
    asm volatile(
        "popl %%ebx\n"                    /* ctx */
        "pushl $dl_lazy_taddr\n"
        "pushl $dl_lazy_saddr\n"
        "pushl %%ebx\n"
        "call dl_lazy_resolve\n"
        "addl $12, %%esp\n"               /* 3 argümanı at */
        "movl dl_lazy_taddr, %%eax\n"
        "movl dl_lazy_saddr, %%ecx\n"
        "movl %%eax, (%%ecx)\n"           /* GOT slot'una hedefi yaz */
        "jmp *%%eax\n"
        : : : "eax", "ecx", "ebx", "memory");
    for (;;) asm volatile("hlt");
}

/* 15C: modül id -> thread-TLS blok ofseti (slot+1 indeksli) */
static unsigned tls_mod_off[DL_MAX];
static unsigned tls_total;
static unsigned tls_main;   /* main thread TLS bloğu (0 = yok) */
static unsigned tls_main_sz;

/* 15D: kopya zonu kayıtları */
static struct {
    char name[24];
    unsigned addr;
    unsigned size;
} dl_copies[DL_COPY_MAX];
static int dl_copies_n;

static int dl_copy_find(const char* name);
static int dl_copy_register(const char* name, unsigned addr, unsigned size);

/* 15D: dışa aktarılan semboller (yükleyici + program) */
struct dl_export { const char* name; void* fn; };

/* ___tls_get_addr: ti[0]=module id, ti[1]=blok içi ofset */
void* ___tls_get_addr(const unsigned* ti);

static struct dl_export dl_builtin_exports[] = {
    { "memcpy",       (void*)dl_memcpy },
    { "memset",       (void*)dl_memset },
    { "___tls_get_addr", (void*)___tls_get_addr },
    { "__tls_get_addr",  (void*)___tls_get_addr },
};
#define DL_BUILTIN_N (sizeof(dl_builtin_exports) / sizeof(dl_builtin_exports[0]))

#define DL_EXPORT_MAX 16
static struct dl_export dl_user_exports[DL_EXPORT_MAX];
static int dl_user_exports_n;

int dl_register_export(const char* name, void* fn) {
    if (!name || !fn) return -1;
    if (dl_user_exports_n >= DL_EXPORT_MAX) return -1;
    for (int i = 0; i < dl_user_exports_n; i++)
        if (dl_strcmp(dl_user_exports[i].name, name) == 0) {
            dl_user_exports[i].fn = fn;
            return 0; /* yeniden kayıt günceller */
        }
    dl_user_exports[dl_user_exports_n].name = name;
    dl_user_exports[dl_user_exports_n].fn = fn;
    dl_user_exports_n++;
    return 0;
}

static void* dl_export_find(const char* name) {
    for (unsigned i = 0; i < DL_BUILTIN_N; i++)
        if (dl_strcmp(dl_builtin_exports[i].name, name) == 0)
            return dl_builtin_exports[i].fn;
    for (int i = 0; i < dl_user_exports_n; i++)
        if (dl_strcmp(dl_user_exports[i].name, name) == 0)
            return dl_user_exports[i].fn;
    return 0;
}

/* 15C: per-thread TLS bloğu (___tls_get_addr'ın kuyruğu) */
void* ___tls_get_addr(const unsigned* ti) {
    (void)ti; /* yığın argümanı yok: i386 TLS ABI GOT çifti adresini EAX'te geçirir */
    unsigned arg;
    asm volatile("" : "=a"(arg)); /* i386 TLS ABI: GOT çifti adresi EAX'te gelir */
    unsigned base = dl_sys(SYS_TLS_GET, 0, 0, 0);
    if (!base) base = (unsigned)dl_tls_main();
    if (!base) return 0;
    unsigned* p = (unsigned*)arg;
    unsigned mod = p[0];
    if (mod == 0 || mod > DL_MAX) return (void*)base;
    return (void*)(base + tls_mod_off[mod - 1] + p[1]);
}

static unsigned elf_hash(const char* s) {
    unsigned h = 0, g;
    while (*s) {
        h = (h << 4) + (unsigned char)*s++;
        g = h & 0xF0000000u;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

static const char* dl_sym_name(struct dl_handle* h, unsigned nameoff) {
    if (!h->strtab || nameoff >= h->strsz) return 0;
    unsigned n = nameoff;
    while (n < h->strsz && h->strtab[n]) n++;
    if (n >= h->strsz) return 0;
    return h->strtab + nameoff;
}

/* bir handle'ın tanımlı sembolünü (hash) çöz */
static void* dl_handle_lookup(struct dl_handle* h, const char* name) {
    if (!h->used || !h->buckets || !h->chains) return 0;
    if (!h->nbucket || !h->nchain || h->nchain > 4096) return 0;
    unsigned hh = elf_hash(name) % h->nbucket;
    unsigned hops = 0;
    for (unsigned i = h->buckets[hh]; i != 0; i = h->chains[i]) {
        if (i >= h->nchain) return 0;
        if (++hops > h->nchain + 1) return 0;
        struct sym* s = &h->symtab[i];
        if (s->shndx == 0) continue; /* tanımsız */
        const char* sn = dl_sym_name(h, s->name);
        if (sn && dl_strcmp(sn, name) == 0)
            return (void*)(h->base + s->value);
    }
    return 0;
}

/* kapsam duyarlı çözümleme: kopya zonu -> exports -> global (ters) -> local (ters).
 * 16F: RTLD_GLOBAL handle'lar RTLD_LOCAL'lardan önce taranır (global öncelik);
 * sınıf içinde en son yüklenen (yüksek seq) kazanır. */
static void* dl_scope_search(struct dl_handle* self, const char* name) {
    int ca = dl_copy_find(name);
    if (ca) return (void*)ca;
    { void* p = dl_export_find(name); if (p) return p; }
    for (int pass = 0; pass < 2; pass++) {
        for (int sq = dl_seq; sq >= 0; sq--) {
            for (int i = 0; i < DL_MAX; i++) {
                struct dl_handle* o = &handles[i];
                if (!o->used || o->seq != sq) continue;
                if (o == self) continue;
                int isg = o->global;
                if ((pass == 0 && !isg) || (pass == 1 && isg)) continue;
                void* a = dl_handle_lookup(o, name);
                if (a) return a;
            }
        }
    }
    return 0;
}

/* tam çözümleme sırası: kopya zonu -> exports -> diğer handle'lar (ters) */
static void* dl_resolve(struct dl_handle* self, const char* name) {
    return dl_scope_search(self, name);
}

/* 16C: RTLD_NEXT — h'den SONRA yüklenen handle'larda sırayla ara */
void* dl_sym_next(void* handle, const char* name) {
    struct dl_handle* h = (struct dl_handle*)handle;
    if (!h || !name) return 0;
    int idx = -1;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == h && h->used) idx = i;
    if (idx < 0) return 0;
    for (int sq = handles[idx].seq + 1; sq <= dl_seq; sq++) {
        for (int i = 0; i < DL_MAX; i++) {
            struct dl_handle* o = &handles[i];
            if (!o->used || o->seq != sq) continue;
            void* a = dl_handle_lookup(o, name);
            if (a) return a;
        }
    }
    return 0;
}

/* 16C: RTLD_DEFAULT — tüm görünür kapsam (self hariç yok) */
static void* dl_resolve_default(const char* name) {
    return dl_scope_search(0, name);
}

static int dl_sym_by_idx(struct dl_handle* h, unsigned idx, unsigned* out) {
    if (idx >= h->nchain) return -1;
    struct sym* s = &h->symtab[idx];
    if (s->shndx != 0) {
        *out = h->base + s->value;
        return 0;
    }
    const char* nm = dl_sym_name(h, s->name);
    if (!nm) return -1;
    if ((s->info >> 4) == STB_WEAK) { *out = 0; return 0; }
    void* p = dl_resolve(h, nm);
    if (!p) return -1;
    *out = (unsigned)p;
    return 0;
}

/* vaddr'i haritalı kullanıcı adresine çevir (bounded) */
static unsigned* dl_ptr(struct dl_handle* h, unsigned vaddr, unsigned need) {
    if (vaddr < h->lo || vaddr + need > h->lo + h->map_len) return 0;
    return (unsigned*)(h->map + (vaddr - h->lo));
}

static int dl_apply_rel(struct dl_handle* h, unsigned rel, unsigned relsz,
                        unsigned relent, int lazy) {
    if (relent != 8) return -1;
    for (unsigned o = 0; o + 8 <= relsz; o += 8) {
        struct rel* r = (struct rel*)dl_ptr(h, rel + o, 8);
        if (!r) return -1;
        unsigned* loc = dl_ptr(h, r->off, 4);
        if (!loc) return -1;
        unsigned type = r->info & 0xFF;
        unsigned idx = r->info >> 8;
        if (type == R_386_NONE) continue;
        else if (type == R_386_RELATIVE) {
            *loc = (unsigned)h->map + *loc;
        } else if (type == R_386_TLS_GD || type == R_386_TLS_DTPMOD32) {
            unsigned mod = 0;
            for (int k = 0; k < DL_MAX; k++)
                if (&handles[k] == h) { mod = (unsigned)(k + 1); break; }
            *loc = mod; /* ofset kelimesi linker tarafından yerleşik */
        } else if (type == R_386_TLS_DTPOFF32) {
            if (idx >= h->nchain) return -1;
            struct sym* s = &h->symtab[idx];
            *loc = (unsigned)s->value + *loc; /* aynı ya da başka modülün bloğu içi ofset */
        } else if (type == R_386_COPY) {
            if (idx >= h->nchain) return -1;
            struct sym* s = &h->symtab[idx];
            const char* nm = dl_sym_name(h, s->name);
            if (!nm) return -1;
            unsigned add = (unsigned)dl_copy_find(nm);
            if (!add) {
                unsigned sz = s->size;
                if (sz > 4096) return -1;
                unsigned slot = dl_sys(SYS_MMAP, 0, 4096,
                    PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
                if (slot == 0xFFFFFFFFu || !slot) return -1;
                if (dl_copy_register(nm, slot, sz) != 0) {
                    dl_sys(SYS_MUNMAP, slot, 4096, 0);
                    return -1;
                }
                void* src = dl_resolve(h, nm);
                if (src) dl_memcpy((void*)slot, src, sz);
                add = slot;
            }
            *loc = add;
        } else {
            unsigned s = 0;
            if (dl_sym_by_idx(h, idx, &s) != 0) {
                return -1;
            }
            if (type == R_386_32) {
                *loc = s + *loc;
            } else if (type == R_386_PC32) {
                *loc = s + (int)*loc - ((unsigned)h->map + r->off - (int)h->lo);
            } else if (type == R_386_GLOB_DAT) {
                *loc = s;
            } else if (type == R_386_JMP_SLOT) {
                /* 16D: lazy modda JMP boş bırakılır; ilk çağrıda trampolin çözer.
                 * (lazy değilse eager: hemen hedef yazılır.) */
                if (!lazy) *loc = s;
            } else return -1;
        }
    }
    return 0;
}

static int dl_copy_find(const char* name) {
    for (int i = 0; i < dl_copies_n; i++)
        if (dl_strcmp(dl_copies[i].name, name) == 0) return dl_copies[i].addr;
    return 0;
}

static int dl_copy_register(const char* name, unsigned addr, unsigned size) {
    if (dl_copies_n >= DL_COPY_MAX) return -1;
    if (dl_copy_find(name)) return -1;
    unsigned n = 0;
    while (name[n] && n < 23) { dl_copies[dl_copies_n].name[n] = name[n]; n++; }
    dl_copies[dl_copies_n].name[n] = 0;
    dl_copies[dl_copies_n].addr = addr;
    dl_copies[dl_copies_n].size = size;
    dl_copies_n++;
    return 0;
}

/* 15G: init/fini dizilerini çalıştır */
static void dl_call_addr(unsigned a) {
    if (!a) return;
    void (*f)(void) = (void (*)(void))a;
    f();
}

static void dl_run_init(struct dl_handle* h) {
    dl_call_addr(h->dt_init);
    for (unsigned i = 0; i < h->init_n; i++) {
        unsigned* e = dl_ptr(h, h->init_arr + 4 * i, 4);
        if (!e) break;
        dl_call_addr(*e);
    }
}

static void dl_run_fini(struct dl_handle* h) {
    unsigned i = h->fini_n;
    while (i > 0) {
        i--;
        unsigned* e = dl_ptr(h, h->fini_arr + 4 * i, 4);
        if (!e) break;
        dl_call_addr(*e);
    }
    dl_call_addr(h->dt_fini);
}

/* 15C: TLS yerleşimini kalan modüllere göre yeniden kur */
static void tls_layout_rebuild(void) {
    unsigned used = 0;
    for (int i = 0; i < DL_MAX; i++) {
        if (!handles[i].used || handles[i].tls_memsz == 0) continue;
        unsigned al = handles[i].tls_align ? handles[i].tls_align : 1;
        unsigned off = (used + al - 1) & ~(al - 1);
        handles[i].tls_off = off;
        tls_mod_off[i] = off;
        used = off + handles[i].tls_memsz;
    }
    tls_total = used;
}

/* 15C: bir TLS bloğu kur (tüm modüllerin ilk verisini kopyalar) */
static unsigned tls_block_make(void) {
    if (tls_total == 0) return 0;
    unsigned totalp = (tls_total + 15) & ~15u;
    if (totalp == 0) totalp = 16;
    unsigned blk = dl_sys(SYS_MMAP, 0, totalp,
        PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
    if (blk == 0xFFFFFFFFu || !blk) return 0;
    dl_memset((void*)blk, 0, totalp);
    for (int i = 0; i < DL_MAX; i++) {
        if (!handles[i].used || handles[i].tls_filesz == 0) continue;
        if (handles[i].tls_init)
            dl_memcpy((void*)(blk + handles[i].tls_off),
                      handles[i].tls_init, handles[i].tls_filesz);
    }
    return blk;
}

void* dl_tls_base(void) {
    unsigned b = dl_sys(SYS_TLS_GET, 0, 0, 0);
    return (void*)b;
}

static void tls_main_drop(void) {
    if (tls_main) {
        dl_sys(SYS_MUNMAP, tls_main, tls_main_sz ? tls_main_sz : 4096, 0);
        tls_main = 0;
        tls_main_sz = 0;
    }
}

void* dl_tls_main(void) {
    if (tls_total == 0) return 0;
    if (tls_main) return (void*)tls_main;
    unsigned totalp = (tls_total + 15) & ~15u;
    if (totalp == 0) totalp = 16;
    unsigned blk = tls_block_make();
    if (!blk) return 0;
    tls_main = blk;
    tls_main_sz = totalp;
    dl_sys(SYS_TLS_SET, blk, 0, 0);
    return (void*)blk;
}

void* dl_tls_new(void) {
    if (tls_total == 0) return 0;
    unsigned blk = tls_block_make();
    if (!blk) return 0;
    dl_sys(SYS_TLS_SET, blk, 0, 0);
    return (void*)blk;
}

unsigned dl_tls_size(void* h) {
    struct dl_handle* hp = (struct dl_handle*)h;
    if (!hp) return 0;
    int mine = 0;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == hp && hp->used) mine = 1;
    if (!mine) return 0;
    return hp->tls_memsz;
}

void* dl_entry(void* h) {
    struct dl_handle* hp = (struct dl_handle*)h;
    if (!hp) return 0;
    int mine = 0;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == hp && hp->used) mine = 1;
    if (!mine) return 0;
    return (void*)(hp->base + hp->entry);
}

/* ---------------- dl_open (16: dl_open_mode) ---------------- */

void* dl_open(const char* path) {
    return dl_open_mode(path, RTLD_NOW);
}

void* dl_open_mode(const char* path, int mode) {
    /* 16H: NULL -> ana program pseudo-tutamacı */
    if (!path) {
        dl_main_ph_init();
        dl_seterr(0);
        return (void*)&main_ph;
    }

    /* 15E: dedupe — aynı isimli handle varsa ref artır, geri dön */
    for (int i = 0; i < DL_MAX; i++) {
        if (handles[i].used && dl_strcmp(handles[i].name, path) == 0) {
            handles[i].refs++;
            dl_seterr(0);
            return (void*)&handles[i];
        }
    }

    int fd = (int)dl_sys(SYS_OPEN, (unsigned)path, O_RDONLY, 0);
    if (fd < 0) { dl_seterr("file not found"); return 0; }
    unsigned img = dl_sys(SYS_MMAP, 0, DL_FILE_MAX,
        PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
    if (img == 0xFFFFFFFFu || !img) {
        dl_seterr("out of memory");
        dl_sys(SYS_CLOSE, (unsigned)fd, 0, 0);
        return 0;
    }
    char* file_tmp = (char*)img;
    /* Lazy mapping: syscall doğrulaması map'li sayfa ister, önce dokun */
    dl_memset(file_tmp, 0, DL_FILE_MAX);
    unsigned total = 0;
    while (total < DL_FILE_MAX) {
        unsigned want = DL_FILE_MAX - total;
        if (want > 4096) want = 4096;
        int n = (int)dl_sys(SYS_READ, (unsigned)fd,
                            (unsigned)(file_tmp + total), want);
        if (n <= 0) break;
        total += (unsigned)n;
        if ((unsigned)n < want) break;
    }
    dl_sys(SYS_CLOSE, (unsigned)fd, 0, 0);

    /* 15F: temel sınırlar */
    if (total < sizeof(struct ehdr)) { dl_seterr("file too short"); goto fail_img; }
    struct ehdr* e = (struct ehdr*)file_tmp;
    if (e->e_ident[0] != 0x7F || e->e_ident[1] != 'E' ||
        e->e_ident[2] != 'L' || e->e_ident[3] != 'F') {
        dl_seterr("not an ELF");
        goto fail_img;
    }
    if (e->e_ident[4] != ELFCLASS32 || e->e_ident[5] != ELFDATA2LSB) {
        dl_seterr("bad ELF class");
        goto fail_img;
    }
    if (e->e_type != ET_DYN) { dl_seterr("not ET_DYN"); goto fail_img; }
    if (e->phnum == 0 || e->phnum > 16) { dl_seterr("bad phdr"); goto fail_img; }
    if (e->phentsize < 32 || e->phentsize > 64) { dl_seterr("bad phdr"); goto fail_img; }
    if (e->phoff > total ||
        (unsigned)e->phnum * e->phentsize > total - e->phoff) {
        dl_seterr("bad phdr");
        goto fail_img;
    }

    unsigned lo = 0xFFFFFFFFu, hi = 0;
    int ndyn = 0;
    unsigned tls_off_f = 0, tls_filesz = 0, tls_memsz = 0, tls_align = 1;
    unsigned phnum = 0;
    struct phdr phdrs16[16];
    for (unsigned i = 0; i < e->phnum; i++) {
        struct phdr* ph = (struct phdr*)(file_tmp + e->phoff + i * e->phentsize);
        if (ph->type == PT_LOAD) {
            if (ph->filesz > ph->memsz) goto fail_img;
            if (ph->offset > total || ph->filesz > total - ph->offset)
                goto fail_img;
            if (ph->vaddr > 0xFFFFFFF0u ||
                ph->memsz > 0xFFFFFFF0u - ph->vaddr)
                goto fail_img;
            if (ph->vaddr < lo) lo = ph->vaddr;
            if (ph->vaddr + ph->memsz > hi) hi = ph->vaddr + ph->memsz;
        } else if (ph->type == PT_DYNAMIC) {
            ndyn++;
        } else if (ph->type == PT_TLS) {
            tls_off_f = ph->offset;
            tls_filesz = ph->filesz;
            tls_memsz = ph->memsz;
            tls_align = ph->align ? ph->align : 1;
        }
        /* 16G: ham phdr kopyası (bounded) */
        if (phnum < 16) {
            phdrs16[phnum] = *ph;
            phnum++;
        }
    }
    if (hi <= lo || ndyn != 1) { dl_seterr("no load segments"); goto fail_img; }
    if (tls_filesz > tls_memsz) goto fail_img;
    if (tls_filesz && (tls_off_f > total || tls_filesz > total - tls_off_f))
        goto fail_img;
    lo &= ~0xFFFu;
    hi = (hi + 0xFFFu) & ~0xFFFu;
    if (hi - lo > 4 * 1024 * 1024) goto fail_img;

    int slot = -1;
    for (int i = 0; i < DL_MAX; i++)
        if (!handles[i].used) { slot = i; break; }
    if (slot < 0) { dl_seterr("no free slot"); goto fail_img; }

    unsigned map = dl_sys(SYS_MMAP, 0, hi - lo,
        PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
    if (map == 0xFFFFFFFFu || !map) { dl_seterr("out of memory"); goto fail_img; }
    dl_memset((void*)map, 0, hi - lo);

    /* Segmentleri yerleştir */
    for (unsigned i = 0; i < e->phnum; i++) {
        struct phdr* ph = (struct phdr*)(file_tmp + e->phoff + i * e->phentsize);
        if (ph->type != PT_LOAD) continue;
        unsigned dst = map + (ph->vaddr - lo);
        unsigned fz = ph->filesz;
        dl_memcpy((void*)dst, file_tmp + ph->offset, fz);
        if (ph->memsz > fz)
            dl_memset((void*)(dst + fz), 0, ph->memsz - fz);
    }

    struct dl_handle* h = &handles[slot];
    dl_memset(h, 0, sizeof(*h));
    h->used = 1;
    h->refs = 1;
    h->base = map - lo;
    h->lo = lo;
    h->map = map;
    h->map_len = hi - lo;
    h->entry = e->entry;
    h->seq = dl_seq++;               /* 16C */
    h->global = (mode & RTLD_GLOBAL) != 0;  /* 16F */
    h->phnum = phnum;                /* 16G */
    dl_memcpy(h->phdrs, phdrs16, phnum * sizeof(struct phdr));
    {
        unsigned n = 0;
        while (path[n] && n < 23) { h->name[n] = path[n]; n++; }
        h->name[n] = 0;
    }
    h->tls_memsz = tls_memsz;
    h->tls_filesz = tls_filesz;
    h->tls_align = tls_align;
    if (tls_filesz) {
        unsigned ic = dl_sys(SYS_MMAP, 0, tls_filesz,
            PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
        if (ic == 0xFFFFFFFFu || !ic) { h->used = 0; goto fail_map; }
        dl_memcpy((void*)ic, file_tmp + tls_off_f, tls_filesz);
        h->tls_init = (const void*)ic;
    }

    /* DYNAMIC tara */
    unsigned dynv = 0;
    for (unsigned i = 0; i < e->phnum; i++) {
        struct phdr* ph = (struct phdr*)(file_tmp + e->phoff + i * e->phentsize);
        if (ph->type == PT_DYNAMIC) { dynv = ph->vaddr; break; }
    }
    {
        struct dyn* d = (struct dyn*)dl_ptr(h, dynv, 8);
        if (!d) { h->used = 0; goto fail_ic; }
        unsigned symtab = 0, strtab = 0, hash = 0, strsz = 0;
        unsigned rel = 0, relsz = 0, relent = 0;
        unsigned jmprel = 0, pltrelsz = 0;
        for (int k = 0; k < 64; k++, d++) {
            if (d->tag == DT_NULL) break;
            switch (d->tag) {
            case DT_NEEDED:                    /* 16E */
                if (h->need_n < DL_NEED_MAX) h->need[h->need_n++] = d->val;
                break;
            case DT_SYMTAB: symtab = d->val; break;
            case DT_STRTAB: strtab = d->val; break;
            case DT_STRSZ: strsz = d->val; break;
            case DT_HASH: hash = d->val; break;
            case DT_REL: rel = d->val; break;
            case DT_RELSZ: relsz = d->val; break;
            case DT_RELENT: relent = d->val; break;
            case DT_JMPREL: jmprel = d->val; break;
            case DT_PLTRELSZ: pltrelsz = d->val; break;
            case DT_INIT: h->dt_init = d->val; break;
            case DT_FINI: h->dt_fini = d->val; break;
            case DT_INIT_ARRAY: h->init_arr = d->val; break;
            case DT_INIT_ARRAYSZ: h->init_n = d->val / 4; break;
            case DT_FINI_ARRAY: h->fini_arr = d->val; break;
            case DT_FINI_ARRAYSZ: h->fini_n = d->val / 4; break;
            default: break;
            }
        }
        if (!symtab || !strtab || !hash || !strsz) {
            h->used = 0;
            goto fail_ic;
        }
        if (symtab < lo || symtab >= hi || strtab < lo || strtab >= hi ||
            hash < lo || hash >= hi) {
            h->used = 0;
            goto fail_ic;
        }
        if (relsz && (rel < lo || rel >= hi)) { h->used = 0; goto fail_ic; }
        if (pltrelsz && (jmprel < lo || jmprel >= hi)) { h->used = 0; goto fail_ic; }

        unsigned* ha = (unsigned*)dl_ptr(h, hash, 8);
        if (!ha) { h->used = 0; goto fail_ic; }
        h->nbucket = ha[0];
        h->nchain = ha[1];
        if (!h->nbucket || !h->nchain || h->nchain > 4096 ||
            h->nbucket > 8192) {
            h->used = 0;
            goto fail_ic;
        }
        h->buckets = ha + 2;
        h->chains = ha + 2 + h->nbucket;
        h->symtab = (struct sym*)(h->base + symtab);
        h->strtab = (const char*)(h->base + strtab);
        h->strsz = strsz;
        h->jmprel = jmprel;                 /* 16D: lazy trampolinleri */
        h->jmprel_n = pltrelsz / 8;

        /* 16E: DT_NEEDED bağımlılıklarını (henüz yüklü değilse) yükle.
         * Sıra önemli: kendi reloc'larından ÖNCE, böylece çözüm hazırdır.
         * Cycle koruması: zaten used olan aynı-isimli handle atlanır. */
        for (unsigned dj = 0; dj < h->need_n; dj++) {
            const char* dn = dl_sym_name(h, h->need[dj]);
            if (!dn || !dn[0]) goto fail_need;
            char fpath[24];
            unsigned fn = 0;
            fpath[0] = '/'; fpath[1] = 'l'; fpath[2] = 'i'; fpath[3] = 'b';
            fpath[4] = '/'; fn = 5;
            {
                unsigned ni = 0;
                while (dn[ni] && fn < 23) fpath[fn++] = dn[ni++];
            }
            fpath[fn] = 0;
            if (dl_strcmp(fpath, path) == 0) continue;  /* kendine istek */
            int dup = 0;
            for (int di = 0; di < DL_MAX; di++)
                if (handles[di].used && dl_strcmp(handles[di].name, fpath) == 0) {
                    dup = 1; break;
                }
            if (dup) continue;
            if (!dl_open_mode(fpath, mode)) { dl_seterr("dependency load failed"); goto fail_need; }
        }

        /* 15C: TLS ofseti ata (başarısızlıkta geri alınır) */
        if (tls_memsz) {
            unsigned al = tls_align;
            unsigned off = (tls_total + al - 1) & ~(al - 1);
            h->tls_off = off;
            tls_mod_off[slot] = off;
            tls_total = off + tls_memsz;
        }

        if (relsz && dl_apply_rel(h, rel, relsz, relent ? relent : 8, 0) != 0)
            goto fail_rel;
        if (pltrelsz &&
            dl_apply_rel(h, jmprel, pltrelsz, 8,
                          (mode & RTLD_LAZY) ? 1 : 0) != 0)
            goto fail_rel;

        /* 16D: RTLD_LAZY — her JMP_SLOT için trampolin üret, GOT'a yaz.
         * İlk çağrıda trampolin dl_lazy_dispatch'e zıplayıp çözer. */
        if ((mode & RTLD_LAZY) && h->jmprel_n) {
            unsigned lm = dl_sys(SYS_MMAP, 0, 4096,
                PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
            if (lm == 0xFFFFFFFFu || !lm) goto fail_rel;
            h->lazy_map = lm;
            h->lazy_use = 0;
            for (unsigned k = 0; k < h->jmprel_n; k++) {
                struct rel* r = (struct rel*)dl_ptr(h, h->jmprel + k * 8, 8);
                if (!r) goto fail_lazy;
                unsigned* loc = dl_ptr(h, r->off, 4);
                if (!loc) goto fail_lazy;
                if (h->lazy_use + 10 > 4096) goto fail_lazy;
                if (k > 255) goto fail_lazy;
                unsigned ta = lm + h->lazy_use;
                unsigned ctx = ((unsigned)(h - &handles[0]) << 8) | k;
                unsigned char* c = (unsigned char*)ta;
                c[0] = 0x68;                       /* push $ctx */
                c[1] = ctx & 0xFF; c[2] = (ctx >> 8) & 0xFF;
                c[3] = (ctx >> 16) & 0xFF; c[4] = (ctx >> 24) & 0xFF;
                c[5] = 0xE9;                       /* jmp rel32 dispatcher */
                long rel8 = (long)(unsigned)dl_lazy_dispatch - (long)(ta + 10);
                c[6] = rel8 & 0xFF; c[7] = (rel8 >> 8) & 0xFF;
                c[8] = (rel8 >> 16) & 0xFF; c[9] = (rel8 >> 24) & 0xFF;
                *loc = ta;
                h->lazy_use += 10;
            }
        }

        /* 15G: init dizilerini reloc sonrası çalıştır */
        dl_run_init(h);

        /* 15C: main blok yeniden kur (yeni TLS modülü eklendi) */
        if (tls_memsz) tls_main_drop();
        if (tls_memsz) dl_tls_main();
    }

    dl_sys(SYS_MUNMAP, img, DL_FILE_MAX, 0);
    dl_seterr(0);
    return (void*)h;

fail_lazy:
    if (h->lazy_map) { dl_sys(SYS_MUNMAP, h->lazy_map, 4096, 0); h->lazy_map = 0; }
fail_need:
fail_rel:
    h->used = 0;
    if (h->tls_init) dl_sys(SYS_MUNMAP, (unsigned)h->tls_init, h->tls_filesz, 0);
    h->tls_init = 0;
    tls_mod_off[slot] = 0;
    tls_layout_rebuild();
    tls_main_drop();
    dl_sys(SYS_MUNMAP, map, hi - lo, 0);
    dl_sys(SYS_MUNMAP, img, DL_FILE_MAX, 0);
    return 0;
fail_ic:
    if (h->tls_init) dl_sys(SYS_MUNMAP, (unsigned)h->tls_init, h->tls_filesz, 0);
fail_map:
    dl_sys(SYS_MUNMAP, map, hi - lo, 0);
fail_img:
    dl_sys(SYS_MUNMAP, img, DL_FILE_MAX, 0);
    return 0;
}

void* dl_sym(void* handle, const char* name) {
    if (!name) return 0;
    if (!handle) return dl_resolve_default(name);   /* 16C: RTLD_DEFAULT */
    struct dl_handle* h = (struct dl_handle*)handle;
    if (h == &main_ph) return dl_export_find(name); /* 16H: ana program */
    int mine = 0;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == h && h->used) mine = 1;
    if (!mine) return 0;
    return dl_handle_lookup(h, name);
}

/* 16B: adresten modül + en yakın tanımlı sembolü bul */
int dladdr(const void* addr, struct dl_info* info) {
    if (!info) return 0;
    dl_memset(info, 0, sizeof(*info));
    unsigned a = (unsigned)addr;
    for (int i = 0; i < DL_MAX; i++) {
        struct dl_handle* h = &handles[i];
        if (!h->used) continue;
        if (a < h->map || a >= h->map + h->map_len) continue;
        info->fname = h->name;
        info->fbase = (void*)h->base;
        unsigned best = 0;
        int has = 0;
        for (unsigned su = 0; su < h->nchain; su++) {
            struct sym* s = &h->symtab[su];
            if (!s->shndx) continue;
            unsigned va = h->base + s->value;
            if (va <= a && (!has || s->value > best)) {
                best = s->value;
                has = 1;
                info->saddr = (void*)va;
                const char* sn = dl_sym_name(h, s->name);
                if (sn) info->sname = sn;
            }
        }
        return 1;
    }
    return 0;
}

/* 16D: isimle eşleşen JMP_SLOT'un GOT içeriği (trampolin ya da hedef) */
unsigned dl_lazy_slot(void* handle, const char* name) {
    struct dl_handle* h = (struct dl_handle*)handle;
    if (!h || !name) return 0;
    int mine = 0;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == h && h->used) mine = 1;
    if (!mine) return 0;
    for (unsigned k = 0; k < h->jmprel_n; k++) {
        struct rel* r = (struct rel*)dl_ptr(h, h->jmprel + k * 8, 8);
        if (!r) return 0;
        unsigned idx = r->info >> 8;
        if (idx >= h->nchain) return 0;
        struct sym* s = &h->symtab[idx];
        const char* sn = dl_sym_name(h, s->name);
        if (sn && dl_strcmp(sn, name) == 0) {
            unsigned* loc = dl_ptr(h, r->off, 4);
            return loc ? *loc : 0;
        }
    }
    return 0;
}

/* 16G: modüllerin program başlıklarını numaralandır */
int dl_iterate_phdr(dl_iter_phdr_cb_t cb, void* arg) {
    if (!cb) return 0;
    struct dl_phdr_info in;
    for (int i = 0; i < DL_MAX; i++) {
        struct dl_handle* h = &handles[i];
        if (!h->used) continue;
        in.name = h->name;
        in.base = (void*)h->base;
        in.phdr = (unsigned*)h->phdrs;
        in.phnum = h->phnum;
        if (cb(&in, sizeof(in), arg)) return 1;
    }
    if (main_ph.used) {                                     /* 16H */
        in.name = main_ph.name;
        in.base = (void*)MAIN_BASE;
        in.phdr = 0;
        in.phnum = 0;
        if (cb(&in, sizeof(in), arg)) return 1;
    }
    return 0;
}

int dl_close(void* handle) {
    struct dl_handle* h = (struct dl_handle*)handle;
    if (!h) return -1;
    if (h == &main_ph) { dl_seterr("can't close main"); return -1; }  /* 16H */
    int idx = -1;
    for (int i = 0; i < DL_MAX; i++)
        if (&handles[i] == h && h->used) idx = i;
    if (idx < 0) return -1;
    if (h->refs > 1) { h->refs--; return 0; } /* 15E */

    /* 15G: fini dizileri ters sırada */
    dl_run_fini(h);

    unsigned map = h->map, map_len = h->map_len;
    unsigned lz = h->lazy_map;
    if (h->tls_init)
        dl_sys(SYS_MUNMAP, (unsigned)h->tls_init, h->tls_filesz, 0);

    /* 15C: TLS yerleşimi yeniden kur */
    dl_memset(h, 0, sizeof(*h));
    tls_mod_off[idx] = 0;
    tls_layout_rebuild();
    tls_main_drop();

    if (lz) dl_sys(SYS_MUNMAP, lz, 4096, 0);   /* 16D */
    dl_sys(SYS_MUNMAP, map, map_len, 0);
    return 0;
}