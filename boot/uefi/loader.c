/* 26A: charOS UEFI bootloader (BOOTX64.EFI).
 *
 * GRUB'un x86_64-efi multiboot2 relocator'ı bu OVMF'te 32-bit kernel'e
 * geçerken kendi imajında #PF yiyor (kernel'den bağımsız, trivial kernelde
 * de aynı imza). Bu yüzden boot yolu bize ait:
 *   1. GOP ile framebuffer kur (1024x768x32, olmazsa mevcut mod)
 *   2. Aynı diskten \boot\kernel.bin (ELF32) oku, LOAD segmentlerini
 *      fiziksel adreslerine yerleştir
 *   3. Multiboot2 bilgi bloğu kur (mmap tag 6 + framebuffer tag 8)
 *   4. ExitBootServices, long mode -> 32-bit protected mode, _start'a atla
 *      (EAX=0x36D76289, EBX=mbi). boot.s aynen çalışır.
 *
 * Derleme: -m64 -mabi=ms (UEFI x64 çağrı kuralı), freestanding, no red zone.
 */

#include "uefi.h"

static EFI_SYSTEM_TABLE *ST;
static EFI_BOOT_SERVICES *BS;
static EFI_HANDLE IM;

/* ---- mini libc ---- */
static void mem_copy(void *d, const void *s, UINTN n) {
    UINT8 *dd = (UINT8*)d; const UINT8 *ss = (const UINT8*)s;
    for (UINTN i = 0; i < n; i++) dd[i] = ss[i];
}
static void mem_zero(void *d, UINTN n) {
    UINT8 *dd = (UINT8*)d;
    for (UINTN i = 0; i < n; i++) dd[i] = 0;
}

/* ASCII -> CHAR16 yazdır (ConOut video+seriye gider) */
static void puts(const char *s) {
    CHAR16 buf[128];
    UINTN i = 0;
    while (*s && i < 127) buf[i++] = (CHAR16)(UINT8)*s++;
    buf[i] = 0;
    ST->ConOut->OutputString(ST->ConOut, buf);
}
static void puthex(UINT64 v) {
    CHAR16 buf[20];
    buf[0]='0'; buf[1]='x';
    for (int i = 0; i < 16; i++) {
        int nib = (v >> (60 - i*4)) & 0xF;
        buf[2+i] = (CHAR16)(nib < 10 ? '0'+nib : 'a'+nib-10);
    }
    buf[18] = 0;
    ST->ConOut->OutputString(ST->ConOut, buf);
}
static void putdec(UINT64 v) {
    CHAR16 buf[24];
    char tmp[24]; int n = 0;
    if (!v) tmp[n++] = '0';
    while (v && n < 23) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < n; i++) buf[i] = (CHAR16)tmp[n-1-i];
    buf[n] = 0;
    ST->ConOut->OutputString(ST->ConOut, buf);
}

/* ---- ELF32 ---- */
#define EI_MAG0 0
#define ELFMAG0 0x7F
#define PT_LOAD 1
typedef struct {
    UINT8  e_ident[16];
    UINT16 e_type, e_machine;
    UINT32 e_version, e_entry, e_phoff, e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf32_Ehdr;
typedef struct {
    UINT32 p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} Elf32_Phdr;

/* ---- Multiboot2 bilgi bloğu ---- */
#define MB2_MAGIC 0x36D76289
#define MAX_MMAP_ENTRIES 48
static UINT8 *mbi_buf;
static UINTN mbi_off;

static void mbi_emit32(UINT32 v) {
    *(UINT32*)(mbi_buf + mbi_off) = v; mbi_off += 4;
}
static void mbi_emit64(UINT64 v) {
    *(UINT64*)(mbi_buf + mbi_off) = v; mbi_off += 8;
}
static void mbi_align8(void) { mbi_off = (mbi_off + 7) & ~7u; }

/* ---- GOP durumu (trampoline sonrası kernel mb2 tag 8'den okur) ---- */
static UINT64 g_fb_base;
static UINT32 g_fb_pitch, g_fb_w, g_fb_h;

/* ---- ACPI RSDP (UEFI config table; kernel mb2 tag 14/15'ten okur) ---- */
static UINT8 g_rsdp_copy[36];
static int g_have_rsdp = 0;

static int guid_eq(const EFI_GUID *a, const EFI_GUID *b) {
    const UINT8 *x = (const UINT8*)a, *y = (const UINT8*)b;
    for (int i = 0; i < 16; i++) if (x[i] != y[i]) return 0;
    return 1;
}

static void find_acpi(void) {
    EFI_CONFIGURATION_TABLE *ct =
        (EFI_CONFIGURATION_TABLE*)ST->ConfigurationTable;
    void *rsdp = 0;
    for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
        if (guid_eq(&ct[i].VendorGuid, &gEfiAcpi20TableGuid)) {
            rsdp = ct[i].VendorTable; break; /* v2 öncelikli */
        }
    }
    if (!rsdp) {
        for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
            if (guid_eq(&ct[i].VendorGuid, &gEfiAcpi10TableGuid)) {
                rsdp = ct[i].VendorTable; break;
            }
        }
    }
    if (!rsdp) { puts("ACPI tablosu yok\r\n"); return; }
    UINT8 *p = (UINT8*)rsdp;
    if (!(p[0]=='R'&&p[1]=='S'&&p[2]=='D'&&p[3]==' '&&
          p[4]=='P'&&p[5]=='T'&&p[6]=='R'&&p[7]==' ')) {
        puts("RSDP imza kotu\r\n"); return;
    }
    for (int i = 0; i < 36; i++) g_rsdp_copy[i] = p[i];
    g_have_rsdp = 1;
    {
        UINT32 rsdt = *(UINT32*)(g_rsdp_copy + 16);
        UINT8 rev = g_rsdp_copy[15];
        UINT64 xsdt = 0;
        for (int i = 0; i < 8; i++)
            xsdt |= (UINT64)g_rsdp_copy[24+i] << (i*8);
        puts("RSDP rev="); putdec(rev);
        puts(" rsdt="); puthex(rsdt);
        puts(" xsdt="); puthex(xsdt); puts("\r\n");
    }
}

/* ---- 64->32 trampoline (tramp.S blob'u <4G tampona kopyalanır) ---- */
extern UINT8 uefi_trampoline_blob[];
extern UINT8 uefi_trampoline_end[];
extern UINT8 slot_fb[];
extern UINT8 slot_fbp[];
extern UINT8 slot_fbw[];
extern UINT8 slot_fbh[];
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_MASK | 5)

typedef void (*tramp_fn)(UINT64 kentry, UINT64 mbi);

/* İlerleme kodları — üç aynalı iz:
 *  - RAM 0x500: QMP 'xp /x 0x500' ile ölüm-sonrası okuma (reset'e kadar yaşar).
 *  - POST port 0x80 (outb): gerçek donanımda POST kartı + QEMU isa-debugcon;
 *    seri, ExitBootServices sonrası ölür ama bu port yaşar.
 *  - g_progress: progress_get() ile o ana kadarki son kod ekrana basılır.
 * progress() saf bellek+port yazmadır (boot-service çağrısı YOK) -> kritik
 * GetMemoryMap/ExitBootServices arasında çağrılması güvenlidir. */
static UINT32 g_progress = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
/* 0x500: gerçek debug scratch adresi (derleyicinin sıfır-boyut uyarısı
 * burada yanlış pozitiftir — adres bilinçli seçilmiş sabittir). */
static void progress(UINT32 code) {
    g_progress = code;
    *(volatile UINT32 *)(UINTN)0x500 = code;
    __asm__ volatile("outb %0, %1" :: "a"((UINT8)code), "Nd"((UINT16)0x80));
}
#pragma GCC diagnostic pop
static UINT32 progress_get(void) { return g_progress; }

/* GOP piksel doldurma — sahne rengi doğrudan ekrana.
 * ExitBootServices SONRASI bile çalışır: long mode + firmware sayfa
 * tabloları hâlâ aktiftir, FB phys adresinden erişilir. ConOut ölmüşken
 * ekrandaki renk, takılma noktasını söyler:
 *   A6=sarı, A7=yeşil, A8=kırmızı, A9=mavi, AA=beyaz */
#define STAGE_A6_COLOR 0x00FFFF00u /* sari */
#define STAGE_A7_COLOR 0x0000FF00u /* yesil */
#define STAGE_A8_COLOR 0x00FF0000u /* kirmizi */
#define STAGE_A9_COLOR 0x000000FFu /* mavi */
#define STAGE_AA_COLOR 0x00FFFFFFu /* beyaz */
static void fb_fill(UINT32 color) {
    if (!g_fb_base || !g_fb_w || !g_fb_h || !g_fb_pitch) return;
    volatile UINT8 *fb = (volatile UINT8 *)(UINTN)g_fb_base;
    for (UINTN y = 0; y < g_fb_h; y++) {
        volatile UINT8 *row = fb + y * g_fb_pitch;
        for (UINTN x = 0; x < g_fb_w; x++) {
            row[x * 4 + 0] = (UINT8)(color & 0xFF);
            row[x * 4 + 1] = (UINT8)((color >> 8) & 0xFF);
            row[x * 4 + 2] = (UINT8)((color >> 16) & 0xFF);
            row[x * 4 + 3] = (UINT8)((color >> 24) & 0xFF);
        }
    }
}
static EFI_STATUS read_file(EFI_FILE_PROTOCOL *root, const CHAR16 *path,
                            void **out_buf, UINTN *out_size) {
    EFI_FILE_PROTOCOL *f = 0;
    EFI_STATUS st = root->Open(root, &f, (CHAR16*)path, 1 /*read*/, 0);
    if (EFI_ERROR(st)) return st;
    UINT8 info_buf[512];
    UINTN info_sz = sizeof(info_buf);
    st = f->GetInfo(f, (EFI_GUID*)&gEfiFileInfoGuid, &info_sz, info_buf);
    if (EFI_ERROR(st)) { f->Close(f); return st; }
    UINT64 fsize = *(UINT64*)(info_buf + 8); /* FileSize */
    void *buf = 0;
    st = BS->AllocatePool(EfiLoaderData, (UINTN)fsize, &buf);
    if (EFI_ERROR(st)) { f->Close(f); return st; }
    UINTN left = (UINTN)fsize;
    st = f->Read(f, &left, buf);
    f->Close(f);
    if (EFI_ERROR(st) || left != (UINTN)fsize) return (EFI_STATUS)(EFI_ERROR_MASK|1);
    *out_buf = buf; *out_size = (UINTN)fsize;
    return EFI_SUCCESS;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable; BS = SystemTable->BootServices; IM = ImageHandle;
    progress(0xA0);

    puts("charOS UEFI loader [26A]\r\n");

    /* ---- 0. ACPI RSDP (config table; mb2 tag 14/15'e konur) ---- */
    find_acpi();

    /* ---- 1. GOP ---- */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;
    EFI_STATUS st = BS->LocateProtocol((EFI_GUID*)&gEfiGraphicsOutputProtocolGuid,
                                       0, (void**)&gop);
    if (EFI_ERROR(st) || !gop) {
        puts("GOP yok, fb'siz devam\r\n");
        g_fb_base = 0;
    } else {
        /* 1024x768 ara, yoksa mevcut modu koru */
        UINT32 want = 0xFFFFFFFF;
        for (UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
            UINTN sz = 0; EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
            if (EFI_ERROR(gop->QueryMode(gop, i, &sz, &info))) continue;
            if (info->HorizontalResolution == 1024 &&
                info->VerticalResolution == 768 &&
                info->PixelFormat < 2) { want = i; break; }
        }
        if (want != 0xFFFFFFFF) {
            if (EFI_ERROR(gop->SetMode(gop, want))) {
                puts("SetMode basarisiz, mevcut mod\r\n");
            }
        }
        g_fb_base = gop->Mode->FrameBufferBase;
        g_fb_w = gop->Mode->Info->HorizontalResolution;
        g_fb_h = gop->Mode->Info->VerticalResolution;
        g_fb_pitch = gop->Mode->Info->PixelsPerScanLine * 4;
        puts("GOP fb="); puthex(g_fb_base); puts(" ");
        putdec(g_fb_w); puts("x"); putdec(g_fb_h);
        puts(" pitch="); putdec(g_fb_pitch); puts("\r\n");
    }
    progress(0xA1);

    /* ---- 2. Kernel dosyasını oku ---- */
    EFI_LOADED_IMAGE_PROTOCOL *li = 0;
    st = BS->HandleProtocol(ImageHandle, (EFI_GUID*)&gEfiLoadedImageProtocolGuid,
                            (void**)&li);
    if (EFI_ERROR(st)) { puts("LoadedImage yok\r\n"); return st; }
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = 0;
    st = BS->HandleProtocol(li->DeviceHandle,
                            (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid,
                            (void**)&sfs);
    if (EFI_ERROR(st)) { puts("SFS yok\r\n"); return st; }
    EFI_FILE_PROTOCOL *root = 0;
    st = sfs->OpenVolume(sfs, &root);
    if (EFI_ERROR(st)) { puts("OpenVolume yok\r\n"); return st; }

    static const CHAR16 kpath[] = {'\\','b','o','o','t','\\','k','e','r','n','e','l','.','b','i','n',0};
    void *kbuf = 0; UINTN ksize = 0;
    st = read_file(root, kpath, &kbuf, &ksize);
    if (EFI_ERROR(st)) { puts("kernel.bin okunamadi\r\n"); return st; }
    puts("kernel.bin ok="); putdec(ksize); puts("B\r\n");
    progress(0xA2);
    root->Close(root); /* ExitBootServices öncesi handle kapatıldı */

    /* ---- 3. ELF32 LOAD ---- */
    Elf32_Ehdr *eh = (Elf32_Ehdr*)kbuf;
    if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F' ||
        eh->e_machine != 3) {
        puts("ELF degil\r\n"); return (EFI_STATUS)(EFI_ERROR_MASK|2);
    }
    Elf32_Phdr *ph = (Elf32_Phdr*)((UINT8*)kbuf + eh->e_phoff);
    for (UINT16 i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz) continue;
        UINT64 base = ph[i].p_paddr & ~0xFFFULL;
        UINT64 end = (ph[i].p_paddr + ph[i].p_memsz + 0xFFFULL) & ~0xFFFULL;
        UINTN pages = (UINTN)((end - base) >> 12);
        UINT64 addr = base;
        st = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &addr);
        if (EFI_ERROR(st)) {
            puts("LOAD yerlesmedi @"); puthex(base); puts("\r\n");
            return st;
        }
        mem_copy((void*)(UINTN)ph[i].p_paddr,
                 (UINT8*)kbuf + ph[i].p_offset, ph[i].p_filesz);
        mem_zero((void*)(UINTN)(ph[i].p_paddr + ph[i].p_filesz),
                 ph[i].p_memsz - ph[i].p_filesz);
    }
    UINT64 kentry = eh->e_entry;
    puts("ELF yerlesti entry="); puthex(kentry); puts("\r\n");
    BS->FreePool(kbuf);
    progress(0xA3);

    /* ---- 4. Trampoline + mbi tamponu (<4G) ---- */
    UINT64 tramp = 0xFFFFFFFFULL;
    st = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, 2, &tramp);
    if (EFI_ERROR(st)) { puts("trampoline alloc yok\r\n"); return st; }
    UINT64 mbib = 0xFFFFFFFFULL;
    st = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, 2, &mbib);
    if (EFI_ERROR(st)) { puts("mbi alloc yok\r\n"); return st; }
    mbi_buf = (UINT8*)(UINTN)mbib;
    mem_zero(mbi_buf, 8192);
    /* trampoline blob'unu <4G tampona kopyala (PIC: her adreste çalışır) */
    {
        UINTN tsize = (UINTN)(uefi_trampoline_end - uefi_trampoline_blob);
        if (!tsize || tsize > 4096) { puts("tramp blob kotu\r\n"); return (EFI_STATUS)(EFI_ERROR_MASK|3); }
        mem_copy((void*)(UINTN)tramp, uefi_trampoline_blob, tsize);
        puts("tramp kopya size="); putdec(tsize); puts("\r\n");
        /* Sahne renkleri için GOP geometrisini blob slotlarına yamala
         * (A8/A9 64-bit+compat mapping ile her FB'ye yazar; AA yalnız <4G) */
        *(UINT64*)((UINTN)tramp + (UINTN)(slot_fb - uefi_trampoline_blob)) = g_fb_base;
        *(UINT32*)((UINTN)tramp + (UINTN)(slot_fbp - uefi_trampoline_blob)) = g_fb_pitch;
        *(UINT32*)((UINTN)tramp + (UINTN)(slot_fbw - uefi_trampoline_blob)) = g_fb_w;
        *(UINT32*)((UINTN)tramp + (UINTN)(slot_fbh - uefi_trampoline_blob)) = g_fb_h;
    }
    progress(0xA4);

    /* ---- 5. Memory map al (ExitBootServices için key) ---- */
    UINTN mmap_sz = 0, map_key = 0, desc_sz = 0;
    UINT32 desc_ver = 0;
    /* önce boyut öğren */
    st = BS->GetMemoryMap(&mmap_sz, 0, &map_key, &desc_sz, &desc_ver);
    mmap_sz += 8 * 48; /* yeni descriptor payı */
    EFI_MEMORY_DESCRIPTOR *mmap = 0;
    st = BS->AllocatePool(EfiLoaderData, mmap_sz, (void**)&mmap);
    if (EFI_ERROR(st)) { puts("mmap pool yok\r\n"); return st; }
    st = BS->GetMemoryMap(&mmap_sz, mmap, &map_key, &desc_sz, &desc_ver);
    if (EFI_ERROR(st)) { puts("mmap yok\r\n"); return st; }

    /* ---- 6. mbi kur ---- */
    mbi_off = 8; /* total_size + reserved */
    /* tag 4: basic meminfo */
    {
        UINT64 top = 0;
        UINTN n = mmap_sz / desc_sz;
        for (UINTN i = 0; i < n; i++) {
            EFI_MEMORY_DESCRIPTOR *d =
                (EFI_MEMORY_DESCRIPTOR*)((UINT8*)mmap + i*desc_sz);
            if (d->Type == EfiConventionalMemory &&
                d->PhysicalStart < 0x100000000ULL) {
                UINT64 e = d->PhysicalStart + d->NumberOfPages*4096ULL;
                if (e > 0x100000000ULL) e = 0x100000000ULL;
                if (e > top) top = e;
            }
        }
        UINT32 upper_k = top > 0x100000 ? (UINT32)((top - 0x100000) >> 10) : 0;
        mbi_emit32(4); mbi_emit32(16);
        mbi_emit32(640); mbi_emit32(upper_k);
    }
    /* tag 6: mmap (yalnız <4G, en fazla 48 girdi) */
    {
        UINTN n = mmap_sz / desc_sz;
        UINTN start = mbi_off;
        mbi_emit32(6); mbi_emit32(0); /* size sonra */
        mbi_emit32(24); mbi_emit32(0); /* entry_size, version */
        UINTN cnt = 0;
        for (UINTN i = 0; i < n && cnt < MAX_MMAP_ENTRIES; i++) {
            EFI_MEMORY_DESCRIPTOR *d =
                (EFI_MEMORY_DESCRIPTOR*)((UINT8*)mmap + i*desc_sz);
            UINT64 b = d->PhysicalStart, l = d->NumberOfPages*4096ULL;
            if (b >= 0x100000000ULL) continue;
            if (b + l > 0x100000000ULL) l = 0x100000000ULL - b;
            UINT32 t = (d->Type == EfiConventionalMemory) ? 1 :
                       (d->Type == EfiACPIReclaimMemory) ? 3 : 2;
            mbi_emit64(b); mbi_emit64(l); mbi_emit32(t); mbi_emit32(0);
            cnt++;
        }
        UINT32 tsize = (UINT32)(mbi_off - start);
        *(UINT32*)(mbi_buf + start + 4) = tsize;
        mbi_align8();
    }
    /* tag 8: framebuffer (GOP; yoksa atla). size=32: gerçek yazılan
     * baytla aynı olmalı, yoksa sonraki tag'ler kayar (26A bulgusu). */
    if (g_fb_base && g_fb_base < 0x100000000ULL && g_fb_pitch &&
        g_fb_w && g_fb_h) {
        mbi_emit32(8); mbi_emit32(32);
        mbi_emit64(g_fb_base);
        mbi_emit32(g_fb_pitch); mbi_emit32(g_fb_w); mbi_emit32(g_fb_h);
        /* bpp=32, type=1 (dogru RGB), reserved */
        mbi_buf[mbi_off++] = 32; mbi_buf[mbi_off++] = 1;
        mbi_buf[mbi_off++] = 0;  mbi_buf[mbi_off++] = 0;
        mbi_align8();
    }
    /* tag 14/15: ACPI RSDP (v1 20B / XSDP 36B kopyası) */
    if (g_have_rsdp) {
        UINT8 rev = g_rsdp_copy[15];
        /* tag 14: v1 alt kümesi (her zaman geçerli) */
        mbi_emit32(14); mbi_emit32(28);
        for (int i = 0; i < 20; i++) mbi_buf[mbi_off++] = g_rsdp_copy[i];
        mbi_align8();
        if (rev >= 2) {
            /* tag 15: tam XSDP */
            mbi_emit32(15); mbi_emit32(44);
            for (int i = 0; i < 36; i++) mbi_buf[mbi_off++] = g_rsdp_copy[i];
            mbi_align8();
        }
    }
    /* end tag */
    mbi_emit32(0); mbi_emit32(8);
    *(UINT32*)mbi_buf = (UINT32)mbi_off; /* total_size */
    *(UINT32*)(mbi_buf + 4) = 0;
    puts("mbi hazir size="); putdec(mbi_off); puts("\r\n");
    puts("tramp="); puthex(tramp); puts(" mbi="); puthex(mbib);
    puts(" kentry="); puthex(kentry); puts("\r\n");
    progress(0xA5);

    /* ---- 7. ExitBootServices + sıçrama ----
     * KRİTİK: GetMemoryMap ile ExitBootServices arasında HİÇBİR
     * boot-service çağrısı (ConOut dahil!) olmamalı.
     * puts ConOut tahsis yapıp haritayı büyütebilir -> tampon kendini
     * büyütür, anahtar her tur tazelenir. */
    /* Ekrana son kod: ConOut hâlâ aktifken zincirin nerede olduğu görülür.
     * Gerçek donanımda ekran fotoğrafındaki bu satır + port 0x80 değeri
     * takılma noktasını verir. */
    puts("kernel'e geciliyor... progress=");
    puthex(progress_get());
    puts("\r\n");
    progress(0xA6);
    fb_fill(STAGE_A6_COLOR);
    {
        int tries = 0;
        for (;;) {
            /* Tur izi: 0xB0+tries (tries 0..7 -> B0..B7). progress() boot-service
             * çağırmaz, GetMemoryMap/ExitBootServices arası güvenlidir. */
            progress(0xB0 + (UINT32)tries);
            st = BS->GetMemoryMap(&mmap_sz, mmap, &map_key, &desc_sz, &desc_ver);
            if (st == EFI_BUFFER_TOO_SMALL) {
                progress(0xBB);
                /* büyüt; bu tahsis SONRAKİ GetMM'den önce, güvenli */
                BS->FreePool(mmap);
                mmap_sz += 64 * 64;
                st = BS->AllocatePool(EfiLoaderData, mmap_sz, (void**)&mmap);
                if (EFI_ERROR(st)) break;
                if (++tries >= 4) break;
                continue;
            }
            if (EFI_ERROR(st)) { progress(0xBC); break; }
            st = BS->ExitBootServices(ImageHandle, map_key);
            if (!EFI_ERROR(st)) break;
            progress(0xBD); /* bayat map_key, döngü başı tazeler */
            if (++tries >= 8) break;
            /* key bayat: döngü başı tazeler (arada çağrı yok) */
        }
    }
    if (EFI_ERROR(st)) {
        progress(0xAE);
        /* Hata kodunun düşük baytını porta aynala (ConOut ölmüş olabilir) */
        __asm__ volatile("outb %0, %1" :: "a"((UINT8)(st & 0xFF)), "Nd"((UINT16)0x80));
        puts("ExitBootServices hata\r\n"); /* en iyi çaba: ConOut ölmüş olabilir */
        return st;
    }
    progress(0xA7);
    /* Boot services kapalı: kopyalanmış trampoline'a git (dönüş yok) */
    /* (adresler önceden basıldı; QMP RIP eşleşsin diye) */
    ((tramp_fn)(UINTN)tramp)(kentry, mbib);
    for (;;) { __asm__ volatile("hlt"); }
}
