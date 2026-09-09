/* 34J: elf64load/cleanup64/acpi64/gop64/nvram64/shim64/runtime64/var64 testi.
 * Calistirma: make test-uefi64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"
#include "elf64.h"
#include "cleanup64.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

/* Sentetik ELF64: 2x PT_LOAD (dosya araligi 0x400'e kadar) */
static unsigned char elfbuf[1024];
static void build_elf(void) {
    Elf64_Ehdr *eh = (Elf64_Ehdr *)elfbuf;
    Elf64_Phdr *ph;
    memset(elfbuf, 0, sizeof(elfbuf));
    eh->e_ident[0] = 0x7F;
    eh->e_ident[1] = 'E';
    eh->e_ident[2] = 'L';
    eh->e_ident[3] = 'F';
    eh->e_ident[4] = 2;
    eh->e_ident[5] = 1;
    eh->e_machine = 62;
    eh->e_entry = 0x100000;
    eh->e_phoff = sizeof(Elf64_Ehdr);
    eh->e_phentsize = sizeof(Elf64_Phdr);
    eh->e_phnum = 2;
    ph = (Elf64_Phdr *)(elfbuf + eh->e_phoff);
    ph[0].p_type = PT_LOAD;
    ph[0].p_offset = 0x100;
    ph[0].p_paddr = 0x100123;
    ph[0].p_filesz = 0x200;
    ph[0].p_memsz = 0x500;
    ph[1].p_type = PT_LOAD;
    ph[1].p_offset = 0x300;
    ph[1].p_paddr = 0x200000;
    ph[1].p_filesz = 0x100;
    ph[1].p_memsz = 0x100;
}

static void fix_sum(unsigned char *b, int len) {
    unsigned int s = 0;
    int i;
    for (i = 0; i < len - 1; i++) s += b[i];
    b[len - 1] = (unsigned char)((256 - (s & 0xFF)) & 0xFF);
}

static unsigned char rsdp[36];
static unsigned char xsdt[36 + 16];
static unsigned char facp[40];
static unsigned char apic_tab[40];

int main(void) {
    struct elf64_layout lay[4];
    int n = 0, i;
    Elf64_Addr entry = 0;
    char name[9];
    unsigned short wname[8];
    unsigned char g1[16], g2[16];

    build_elf();
    CHECK(elf64_check(elfbuf, sizeof(elfbuf), &entry) == 0 &&
          entry == 0x100000, "34A elf64 check");
    CHECK(elf64_layout(elfbuf, sizeof(elfbuf), lay, 4, &n) == 0 && n == 2,
          "34A layout 2 segment");
    CHECK(lay[0].paddr == 0x100000 && lay[0].memsz == 0x1000,
          "34A hizalama");
    CHECK(elf64_layout(elfbuf, sizeof(elfbuf), lay, 1, &n) != 0,
          "34A tasma red");
    CHECK(elf64_layout("BOZUK", 5, lay, 4, &n) != 0, "34A bozuk red");

    cleanup64_clear();
    CHECK(cleanup64_track((void *)0x1000) == 0, "34B kayit");
    CHECK(cleanup64_track((void *)0x1000) != 0, "34B cift red");
    CHECK(cleanup64_track((void *)0x2000) == 0, "34B kayit2");
    CHECK(cleanup64_count() == 2, "34B sayac");
    CHECK(cleanup64_get(0) == (void *)0x1000, "34B oku");
    cleanup64_untrack((void *)0x1000);
    CHECK(cleanup64_count() == 1, "34B cikar");
    for (i = 0; i < 20; i++) cleanup64_track((void *)(0x3000UL + (u64)i));
    CHECK(cleanup64_count() == CLEANUP64_MAX, "34B cap");

    memset(rsdp, 0, sizeof(rsdp));
    memcpy(rsdp, "RSD PTR ", 8);
    rsdp[15] = 2;
    fix_sum(rsdp, 20);
    fix_sum(rsdp, 36);
    CHECK(acpi64_rsdp_check(rsdp, 36) == 0, "34C rsdp ok");
    rsdp[10] ^= 0xFF;
    CHECK(acpi64_rsdp_check(rsdp, 36) != 0, "34C bozuk rsdp red");
    rsdp[10] ^= 0xFF;

    memset(xsdt, 0, sizeof(xsdt));
    memcpy(xsdt, "XSDT", 4);
    xsdt[4] = 36 + 16;
    memset(facp, 0, sizeof(facp));
    memcpy(facp, "FACP", 4);
    memset(apic_tab, 0, sizeof(apic_tab));
    memcpy(apic_tab, "APIC", 4);
    for (i = 0; i < 8; i++)
        xsdt[36 + i] = (unsigned char)(((u64)facp >> (i * 8)) & 0xFF);
    for (i = 8; i < 16; i++)
        xsdt[36 + i] = (unsigned char)(((u64)apic_tab >> ((i - 8) * 8)) & 0xFF);
    {
        u64 addr = 0;
        CHECK(acpi64_xsdt_entry(xsdt, sizeof(xsdt), 0, &addr) == 0 &&
              addr == (u64)facp, "34C xsdt girdi");
        CHECK(acpi64_xsdt_entry(xsdt, sizeof(xsdt), 5, &addr) != 0,
              "34C asim red");
        CHECK(acpi64_find(xsdt, sizeof(xsdt), "FACP", &addr) == 0 &&
              addr == (u64)facp, "34C facp bul");
        CHECK(acpi64_find(xsdt, sizeof(xsdt), "HPET", &addr) != 0,
              "34C yok red");
    }

    CHECK(gop64_validate(1024, 768, 1024, 0) == 0, "34D mod ok");
    CHECK(gop64_validate(1024, 768, 1000, 0) != 0, "34D pitch red");
    CHECK(gop64_validate(0, 768, 1024, 0) != 0, "34D sifir red");
    CHECK(gop64_validate(5000, 768, 5000, 0) != 0, "34D asiri red");
    CHECK(gop64_size_bytes(768, 1024) == 768ULL * 1024 * 4, "34D boyut");
    CHECK(gop64_pages(768, 1024) == (768ULL * 1024 * 4 + 0xFFF) / 0x1000,
          "34D sayfa");
    CHECK(gop64_fill_color(0x112233, 0) == 0x112233, "34D rgb ayni");
    CHECK(gop64_fill_color(0x112233, 1) == 0x332211, "34D bgr takas");

    CHECK(nvram64_boot_name(1, name) == 0 &&
          strcmp(name, "Boot0001") == 0, "34E ad");
    CHECK(nvram64_boot_name(0xABCD, name) == 0 &&
          strcmp(name, "BootABCD") == 0, "34E ad hex");
    CHECK(nvram64_boot_name(0x10000, name) != 0, "34E asim red");
    CHECK(nvram64_attrs_valid(0x7) == 0, "34E oznitelik ok");
    CHECK(nvram64_attrs_valid(0x2) != 0, "34E eksik red");

    CHECK(shim64_policy(0, 0, 0) == 1, "34F kapali serbest");
    CHECK(shim64_policy(1, 1, 0) == 1, "34F setup serbest");
    CHECK(shim64_policy(1, 0, 1) == 2, "34F denetimli");
    CHECK(shim64_policy(1, 0, 0) == 0, "34F zorunlu engel");

    CHECK(runtime64_keep(5) && runtime64_keep(6), "34G runtime korunur");
    CHECK(runtime64_keep(11), "34G mmio korunur");
    CHECK(!runtime64_keep(7) && !runtime64_keep(9), "34G diger atilir");

    wname[0] = 'S'; wname[1] = 'e'; wname[2] = 'c';
    wname[3] = 'u'; wname[4] = 'r'; wname[5] = 'e';
    wname[6] = 'B'; wname[7] = 0;
    /* 8 uzunluk tamponu terminatorsuz test icin ayri */
    CHECK(var64_name_ok(wname) == 0, "34I ad ok");
    wname[0] = 0;
    CHECK(var64_name_ok(wname) != 0, "34I bos red");
    CHECK(var64_name_ok(0) != 0, "34I null red");
    CHECK(var64_size_ok(100, 1024) == 0, "34I boyut ok");
    CHECK(var64_size_ok(2048, 1024) != 0, "34I asiri red");
    for (i = 0; i < 16; i++) { g1[i] = (unsigned char)i; g2[i] = (unsigned char)i; }
    CHECK(var64_guid_eq(g1, g2) == 1, "34I guid esit");
    g2[15] ^= 1;
    CHECK(var64_guid_eq(g1, g2) == 0, "34I guid farkli");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
