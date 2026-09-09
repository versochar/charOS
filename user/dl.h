#ifndef USER_DL_H
#define USER_DL_H

/* 15A-15H: çalışır-zaman dinamik yükleyici (ET_DYN, sysv-hash, eager bind).
 * 15C: per-thread TLS (DTPMOD/GD ___tls_get_addr), 15D: interposition +
 * program exports + COPY, 15E: refcount/dedupe (fork CoW paylaşımı),
 * 15F: sağlamlık (sınır denetimleri + geri alma), 15G: init/fini dizileri,
 * 15H: ET_DYN (PIE) giriş çalıştırma (dl_entry).
 * 16A: dlerror, 16B: dladdr, 16C: RTLD_DEFAULT/RTLD_NEXT kapsamı,
 * 16D: lazy PLT (RTLD_LAZY trampolin), 16E: DT_NEEDED transitif yükleme,
 * 16F: RTLD_GLOBAL/LOCAL görünürlük, 16G: dl_iterate_phdr,
 * 16H: dl_open(NULL) ana program tutamacı + temiz kapatma. */

#define RTLD_LAZY    0x1
#define RTLD_NOW     0x2
#define RTLD_GLOBAL  0x4

void* dl_open(const char* path);       /* tutamaç / 0 hata (RTLD_NOW) */
void* dl_open_mode(const char* path, int mode); /* 16: mode bayrakları */
void* dl_sym(void* h, const char* name); /* adres / 0 yok (NULL = varsayılan kapsam) */
int   dl_close(void* h);               /* 0 ok */
void* dl_entry(void* h);               /* 15H: modül giriş noktası (base+e_entry) */
unsigned dl_tls_size(void* h);         /* 15C: modülün PT_TLS blok boyutu */

/* 16A: en son hatayı döndürür ve temizler (yoksa 0) */
const char* dlerror(void);

/* 16B: adres -> (dosya, taban, rastlaştırılmış sembol + ofset) */
struct dl_info { const char* fname; void* fbase; const char* sname; void* saddr; };
int dladdr(const void* addr, struct dl_info* info);

/* 16C: kapsama duyarlı arama. NULL handle = RTLD_DEFAULT (en son yüklenen önce). */
void* dl_sym_next(void* h, const char* name); /* h'den SONRA yüklenenlerde ara */

/* 16D: lazy JMP_SLOT'un o anki GOT değeri (test için; 0 = yok/hatalı) */
unsigned dl_lazy_slot(void* h, const char* name);

/* 16G: modül program başlıklarını numaralandır */
struct dl_phdr_info {
    const char* name;
    void* base;
    unsigned* phdr;   /* ham phdr kayıtları (bun adet, 32B adet) */
    unsigned phnum;
};
typedef int (*dl_iter_phdr_cb_t)(struct dl_phdr_info*, unsigned size, void* arg);
int dl_iterate_phdr(dl_iter_phdr_cb_t cb, void* arg);

/* 15D: ana program/uygulama tarafından dışa aktarılan fonksiyonları kaydet.
 * Yüklenen .so'lardaki tanımsız semboller önce bu tabloda aranır. */
int dl_register_export(const char* name, void* fn);

/* 15C: per-thread TLS bloğu yönetimi */
void* dl_tls_base(void);               /* SYS_TLS_GET */
void* dl_tls_new(void);                /* yeni thread bloğu (SYS_TLS_SET kurulur) */
void* dl_tls_main(void);               /* main thread bloğu (lazik + set) */

#endif