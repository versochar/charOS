/* 29.3: Syscall belge tablosu (nr sıralı, sayı doc_selftest ile denetlenir).
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "core/doc.h"
#include "core/verify.h"

typedef struct {
    uint32_t nr;
    const char* name;
    const char* desc;
} doc_entry_t;

static const doc_entry_t dtab[] = {
    {0, "EXIT", "cikis kodu ile sonlan"},
    {1, "WRITE", "fd'ye yaz"},
    {2, "READ", "fd'den oku"},
    {3, "GETPID", "pid al"},
    {4, "YIELD", "gorev birak"},
    {5, "GETTICKS", "zaman sayaci"},
    {10, "OPEN", "dosya ac"},
    {11, "CLOSE", "fd kapat"},
    {12, "FORK", "surec cogalt"},
    {13, "PIPE", "boru kur"},
    {14, "SIGNAL", "sinyal yakala"},
    {15, "EXEC", "program yukle"},
    {16, "NANOSLEEP", "kisa uyku"},
    {17, "KILL", "sinyal gonder"},
    {18, "SIGRETURN", "sinyalden don"},
    {19, "WAITPID", "cocuk bekle"},
    {20, "SEM_INIT", "semafor kur"},
    {21, "SEM_WAIT", "semafor bekle"},
    {22, "SEM_POST", "semafor birak"},
    {23, "SEM_DESTROY", "semafor yoket"},
    {24, "NICE", "oncelik ayarla"},
    {90, "MMAP", "bellek esle"},
    {91, "MUNMAP", "eslemeyi kaldir"},
    {130, "CLONE", "iplik klonla"},
    {131, "BLKREAD", "sektor oku"},
    {132, "BLKWRITE", "sektor yaz (RAWIO)"},
    {133, "DFSAVE", "diskfs yaz"},
    {134, "DFLOAD", "diskfs oku"},
    {135, "BLKINFO", "sektor sayisi"},
    {136, "INPUT", "girdi olayi al"},
    {137, "FBINFO", "fb modu al"},
    {138, "BRK", "heap siniri"},
    {139, "FUTEX", "bekle/uyandir"},
    {140, "GETUID", "uid al"},
    {141, "SETUID", "uid ayarla (SETUID)"},
    {142, "GETGID", "gid al"},
    {143, "SETGID", "gid ayarla (SETGID)"},
    {144, "SYSLOG", "halka tampon"},
    {145, "UPTIME", "saniye"},
    {146, "CHMOD", "dosya modu"},
    {147, "DUP2", "fd yonlendir"},
    {148, "TLS_SET", "TLS tabani ata"},
    {149, "TLS_GET", "TLS tabani al"},
    {150, "DFMKDIR", "dizin kur"},
    {151, "DFREMOVE", "sil"},
    {152, "DFTRUNCATE", "sifirla"},
    {153, "DFSTAT", "boyut/tur"},
    {154, "DFLIST", "listele"},
    {155, "DFCHECK", "butunluk"},
    {156, "LSDIR", "chfs listele"},
    {157, "VTSWITCH", "terminal degistir"},
    {158, "TASKLIST", "surec listesi"},
    {159, "GETGROUPS", "gruplari al"},
    {160, "SETGROUPS", "gruplari ata (SETGID)"},
    {161, "CAPGET", "yetki maskesi al"},
    {162, "CAPSET", "yetki kis"},
    {163, "SB_ALLOW", "sandbox serbest"},
    {164, "SB_DENY", "sandbox engelle"},
    {165, "SB_ON", "sandbox ac"},
    {166, "DOCNAME", "syscall adi"},
    {167, "DOCDESC", "syscall aciklamasi"},
};

#define DOC_N (sizeof(dtab) / sizeof(dtab[0]))

/* 29.3: derleme-zamanı kanıtı */
STATIC_ASSERT(DOC_N > 50);

uint32_t doc_count(void) {
    return DOC_N;
}

static const doc_entry_t* doc_find(uint32_t nr) {
    for (uint32_t i = 0; i < DOC_N; i++) {
        if (dtab[i].nr == nr) return &dtab[i];
    }
    return 0;
}

const char* doc_name(uint32_t nr) {
    const doc_entry_t* e = doc_find(nr);
    return e ? e->name : 0;
}

const char* doc_desc(uint32_t nr) {
    const doc_entry_t* e = doc_find(nr);
    return e ? e->desc : 0;
}

int doc_copy(const char* s, char* out, uint32_t max) {
    uint32_t i = 0;
    if (REQUIRE(s != 0 && out != 0 && max > 0, 0xF101) != 0) return -1;
    while (s[i] != '\0') {
        if (REQUIRE(i + 1 < max, 0xF102) != 0) return -1; /* NUL payı */
        out[i] = s[i];
        i++;
    }
    out[i] = '\0';
    return (int)i;
}

int doc_selftest(void) {
    /* sıralılık + boş-ad yok + sayı tutarlılığı */
    for (uint32_t i = 1; i < DOC_N; i++) {
        if (dtab[i].nr <= dtab[i - 1].nr) return -1;
    }
    for (uint32_t i = 0; i < DOC_N; i++) {
        if (!dtab[i].name || !dtab[i].name[0]) return -2;
        if (!dtab[i].desc || !dtab[i].desc[0]) return -3;
    }
    if (doc_count() != DOC_N) return -4;
    if (!doc_name(1) || doc_name(999)) return -5;
    return 0;
}
