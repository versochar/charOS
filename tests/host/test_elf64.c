/* 52J: elfparse64/progheader64/dynlink64/soload64/aslruser64/rpath64/
 *      delayload64/symver64/dlopen64 host testi.
 * Calistirma: make test-elf64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"
#include "elf64.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static unsigned char soimg[2048];

static void wr16(unsigned char *p, u32 v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void wr64(unsigned char *p, u64 v) {
    int i;
    for (i = 0; i < 8; i++) p[i] = (unsigned char)((v >> (i * 8)) & 0xFF);
}

/* Sentetik ET_DYN: LOAD + DYNAMIC + INTERP, 1 bolum */
static void build_so(void) {
    unsigned char *e = soimg;
    unsigned char *d;
    memset(soimg, 0, sizeof(soimg));
    e[0] = 0x7F;
    e[1] = 'E';
    e[2] = 'L';
    e[3] = 'F';
    e[4] = 2;
    e[5] = 1;
    wr16(e + 16, 3);          /* ET_DYN */
    wr16(e + 18, 62);         /* x86-64 */
    wr64(e + 24, 0x1000);     /* entry */
    wr64(e + 32, 64);         /* phoff */
    wr16(e + 54, 56);         /* phentsize */
    wr16(e + 56, 3);          /* phnum */
    wr64(e + 40, 0x500);      /* shoff */
    wr16(e + 60, 1);          /* shnum */
    /* PH0 LOAD */
    wr64(e + 64 + 0, 1);
    wr64(e + 64 + 8, 0);
    wr64(e + 64 + 16, 0);
    wr64(e + 64 + 32, 0x400);
    wr64(e + 64 + 40, 0x400);
    wr64(e + 64 + 48, 5);
    /* PH1 DYNAMIC */
    wr64(e + 120 + 0, 2);
    wr64(e + 120 + 8, 0x300);
    wr64(e + 120 + 16, 0x300);
    wr64(e + 120 + 32, 5 * 16);
    wr64(e + 120 + 40, 5 * 16);
    /* PH2 INTERP */
    wr64(e + 176 + 0, 3);
    wr64(e + 176 + 8, 0x200);
    wr64(e + 176 + 16, 0x200);
    wr64(e + 176 + 32, 14);
    wr64(e + 176 + 40, 14);
    memcpy(soimg + 0x200, "/lib/ld64.so.1", 14);
    d = soimg + 0x300;
    wr64(d + 0, 1);
    wr64(d + 8, 1);   /* NEEDED -> str 1 */
    wr64(d + 16, 14);
    wr64(d + 24, 9);  /* SONAME -> str 9 */
    wr64(d + 32, 5);
    wr64(d + 40, 0x400); /* STRTAB */
    wr64(d + 48, 6);
    wr64(d + 56, 32);    /* STRSZ */
    wr64(d + 64, 0);
    wr64(d + 72, 0); /* NULL */
    memcpy(soimg + 0x400, "\0libc.so\0libx.so\0", 17);
    /* SHDR[0] */
    wr64(soimg + 0x500 + 24, 0x600);
    wr64(soimg + 0x500 + 32, 16);
    wr16(soimg + 0x500 + 4, 1); /* PROGBITS */
}

static int each_count = 0;

static int each_cb(u32 type, u64 off, u64 vaddr, u64 filesz, u64 memsz,
                   u32 flags) {
    (void)off;
    (void)vaddr;
    (void)filesz;
    (void)memsz;
    (void)flags;
    each_count++;
    if (type == 3) return 0;
    return 0;
}

int main(void) {
    struct elfparse64_hdr hdr;
    u64 off = 0, sz = 0;
    u32 stype = 0;
    char ibuf[32], sbuf[32];
    char needed[4][32];
    struct soload64_seg segs[4];
    int nsegs = 0;
    u64 total = 0;

    build_so();
    CHECK(elfparse64_header(soimg, sizeof(soimg), &hdr) == 0 &&
          hdr.type == 3 && hdr.machine == 62 && hdr.entry == 0x1000 &&
          hdr.phnum == 3 && hdr.shnum == 1, "52A baslik");
    CHECK(elfparse64_header(soimg, 10, 0) != 0, "52A kisa red");
    CHECK(elfparse64_section(soimg, sizeof(soimg), 0, &off, &sz,
                             &stype) == 0 && off == 0x600 && sz == 16 &&
          stype == 1, "52A bolum");
    CHECK(elfparse64_section(soimg, sizeof(soimg), 5, 0, 0, 0) != 0,
          "52A asim red");

    each_count = 0;
    CHECK(progheader64_each(soimg, sizeof(soimg), each_cb) == 3 &&
          each_count == 3, "52B yurume");
    CHECK(progheader64_each(soimg, sizeof(soimg), 0) != 0,
          "52B cb red");
    CHECK(progheader64_interp(soimg, sizeof(soimg), ibuf, sizeof(ibuf)) ==
              0 && !strcmp(ibuf, "/lib/ld64.so.1"), "52B interp");
    CHECK(progheader64_stack_exec(soimg, sizeof(soimg)) == 0,
          "52B NX yigin");

    {
        /* Dinamik dizi: NEEDED, SONAME, STRTAB, STRSZ, NULL */
        long tags[5] = {1, 14, 5, 6, 0};
        u64 vals[5] = {1, 9, 0x400, 32, 0};
        struct {
            long tag;
            u64 val;
        } dyn[5];
        const char *strtab = (const char *)soimg + 0x400;
        int i;
        for (i = 0; i < 5; i++) {
            dyn[i].tag = tags[i];
            dyn[i].val = vals[i];
        }
        {
            u64 v = 0;
            CHECK(dynlink64_get(dyn, 5, 14, &v) == 0 && v == 9,
                  "52C girdi");
            CHECK(dynlink64_get(dyn, 5, 99, 0) != 0, "52C yok red");
        }
        CHECK(dynlink64_needed(dyn, 5, strtab, 32, needed, 4) == 1 &&
              !strcmp(needed[0], "libc.so"), "52C needed");
        CHECK(dynlink64_str(dyn, 5, 14, strtab, 32, sbuf, sizeof(sbuf)) ==
                  0 && !strcmp(sbuf, "libx.so"), "52C soname");
        CHECK(dynlink64_str(dyn, 5, 15, strtab, 32, sbuf, sizeof(sbuf)) !=
                  0, "52C rpath yok");
    }

    CHECK(soload64_plan(soimg, sizeof(soimg), 0x1000000ULL, segs, 4,
                        &nsegs, &total) == 0 && nsegs == 1 &&
          segs[0].vaddr == 0x1000000ULL && total == 0x400, "52D plan");
    CHECK(soload64_plan(soimg, sizeof(soimg), 0x123, segs, 4, 0, 0) != 0,
          "52D hizasiz red");
    CHECK(soload64_plan(soimg, sizeof(soimg), 0x1000, segs, 0, 0, 0) !=
              0, "52D tampon red");

    CHECK((aslruser64_mmap_base(0x1111) & 0x1FFFFFULL) == 0,
          "52E mmap hiza");
    CHECK(aslruser64_mmap_base(0x1111) != aslruser64_mmap_base(0x2222),
          "52E mmap dagilim");
    CHECK((aslruser64_stack(0x1111) & 0xF) == 0, "52E yigin hiza");
    {
        u64 b = aslruser64_brk(0x1111);
        CHECK(b >= 0x20000000ULL && b < 0x24000000ULL, "52E brk aralik");
    }
    {
        u64 pb = aslruser64_pie_bias(0x1111);
        CHECK(pb < (1ULL << 30) && (pb & 0x1FFFFFULL) == 0,
              "52E pie bias");
    }

    CHECK(rpath64_add_file("/opt/lib", "libx.so") == 0, "52F dosya");
    CHECK(rpath64_add_file("/usr/lib", "libc.so") == 0, "52F dosya2");
    {
        char dir[64];
        CHECK(rpath64_find("/opt/lib", 0, 0, "libx.so", dir,
                           sizeof(dir)) == 0 &&
              !strcmp(dir, "/opt/lib"), "52F rpath");
        CHECK(rpath64_find(0, 0, "/usr/lib:/lib", "libc.so", dir,
                           sizeof(dir)) == 0 &&
              !strcmp(dir, "/usr/lib"), "52F ldpath");
        CHECK(rpath64_find(0, "/run", 0, "libx.so", dir, sizeof(dir)) !=
                  0, "52F runpath yok");
        CHECK(rpath64_find(0, 0, 0, "yok.so", 0, 0) != 0, "52F yok red");
    }

    CHECK(delayload64_register("foo") == 0, "52G kayit");
    CHECK(delayload64_call("foo") == 0, "52G cozumsuz sifir");
    CHECK(delayload64_resolve("foo", 0x1234) == 0, "52G coz");
    CHECK(delayload64_call("foo") == 0x1234, "52G cagri");
    CHECK(delayload64_resolve("yok", 1) != 0, "52G kayitsiz red");

    CHECK(symver64_add_def("printf", "LIBC_2.1") == 0, "52H tanim");
    CHECK(symver64_need("printf", "LIBC_2.0") == 0, "52H istek");
    CHECK(symver64_check() == 0, "52H saglandi");
    CHECK(symver64_need("open", "LIBC_2.0") == 0, "52H istek2");
    CHECK(symver64_check() == 1, "52H 1 eksik");
    CHECK(symver64_add_def("open", "LIBC_2.5") == 0, "52H tanim2");
    CHECK(symver64_check() == 0, "52H tamam");

    {
        int h = dlopen64_open("/lib/libx.so", 1);
        int h2;
        CHECK(h >= 0, "52I ac");
        CHECK(dlopen64_add_sym(h, "x_init", 0x5000) == 0, "52I sembol");
        CHECK(dlopen64_sym(h, "x_init") == 0x5000, "52I coz");
        CHECK(dlopen64_sym(h, "yok") == 0, "52I yok sifir");
        h2 = dlopen64_open("/lib/liby.so", 0);
        CHECK(dlopen64_add_sym(h2, "ysh", 0x6000) == 0, "52I ozel");
        CHECK(dlopen64_sym(h2, "x_init") == 0x5000, "52I global yedek");
        CHECK(dlopen64_close(h) == 0, "52I kapat");
        CHECK(dlopen64_close(h) != 0, "52I cift kapat red");
        CHECK(dlopen64_close(h2) == 0, "52I kapat2");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
