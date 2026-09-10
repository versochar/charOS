#include <core/gdt.h>
#include <string.h>
#include <core/idt.h>
#include <core/isr.h>
#include <core/pic.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <drivers/keyboard.h>
#include <process/shell.h>
#include <process/task.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <memory/mb2.h>
#include <memory/kheap.h>
#include <drivers/rtc.h>
#include <core/syscall.h>
#include <core/verify.h>
#include <core/trace.h>
#include <core/prof.h>
#include <core/apic.h>
#include <fs/chfs.h>
#include <process/signal.h>
#include <process/pipe.h>
#include <fs/proc.h>
#include <net/net.h>
#include <net/tcp.h>
#include <drivers/pci.h>
#include <drivers/virtio_blk.h>
#include <drivers/xhci.h>
#include <drivers/usbhid.h>
#include <drivers/nvme.h>
#include <drivers/gpt.h>
#include <drivers/hda.h>
#include <drivers/clock.h>
#include <drivers/timer_queue.h>
#include <drivers/cpu_features.h>
#include <drivers/thermal.h>
#include <drivers/desktop_v2.h>
#include <drivers/dual_gpu.h>
#include <drivers/app_platform.h>
#include <drivers/pkg_mgr.h>
#include <core/boot_flow.h>
#include <test/hardware.h>
#include <drivers/panic.h>
#include <drivers/perf.h>
#include <drivers/network_note.h>
#include <drivers/release.h>
#include <drivers/sysmon.h>
#include <drivers/input.h>
#include <drivers/fb.h>
#include <drivers/gfx.h>
#include <drivers/compositor.h>
#include <drivers/wm.h>
#include <drivers/wserver.h>
#include <drivers/widget.h>
#include <drivers/shell.h>
#include <drivers/menu.h>
#include <drivers/term.h>
#include <drivers/fm.h>
#include <drivers/inr.h>
#include <drivers/font.h>
#include <drivers/clip.h>
#include <drivers/vt.h>
#include <drivers/theme.h>
#include <memory/brk.h>
#include <process/pty.h>
#include <process/futex.h>
#include <core/syslog.h>
#include <drivers/apps.h>
#include <drivers/de.h>
#include <drivers/mouse.h>
#include <drivers/acpi.h>
#include <drivers/dev.h>
#include <fs/diskfs.h>

/* Multiboot header bilgisini sakla (global değişkenler) */
uint32_t multiboot_magic = 0;
uint32_t multiboot_ptr = 0;

/* Test task'ları - 4A/4B için */
static void task_a(void) {
    for(int i=0;i<3;i++) {
        serial_puts("[task_a] tick "); serial_puthex(timer_get_ticks()); serial_puts("\n");
        // vga_puts ile shell'i bozmayalım
        timer_wait(50);
    }
    serial_puts("[task_a] done\n");
    // task_exit çağrılacak (wrapper)
}
static void task_b(void) {
    for(int i=0;i<3;i++) {
        serial_puts("[task_b] tick "); serial_puthex(timer_get_ticks()); serial_puts("\n");
        timer_wait(70);
    }
    serial_puts("[task_b] done\n");
}
/* 25.4: trace arka ucu syslog'a yazar */
static void trace_kbackend(uint32_t level, const char* tag, const char* msg) {
    (void)level; (void)tag;
    syslog_puts(msg);
}
/* 24.4 + 25.4: sözleşme ihlalleri trace (WARN) + serial'e düşer */
static void verify_khook(uint32_t code, const char* file, uint32_t line) {
    (void)file; (void)line;
    trace_event(TRACE_WARN, "verify", "[VERIFY] contract ihlali");
    serial_puts("[VERIFY] ihlal kod "); serial_puthex(code); serial_puts("\n");
}
void kernel_main(uint32_t magic, uint32_t mboot_ptr)
{
    multiboot_magic = magic;
    multiboot_ptr = mboot_ptr;

    serial_init();

    /* 26A: UEFI/multiboot2 -> mb1 normalizasyonu (pmm/fb aynı düzende okur) */
    mboot_ptr = boot_info(magic, mboot_ptr);
    multiboot_ptr = mboot_ptr;

    gdt_init();
    vga_init();

    vga_puts("========================================\n");
    vga_puts("       charOS - 32-bit x86 Kernel       \n");
    vga_puts("========================================\n");
    vga_puts("Booted successfully!\n");
    vga_puts("Multiboot magic: 0x"); vga_puthex(magic); vga_puts("\n");
    vga_puts("Multiboot info ptr: 0x"); vga_puthex(mboot_ptr); vga_puts("\n");
    serial_puts("========================================\n");
    serial_puts("       charOS - 32-bit x86 Kernel       \n");
    serial_puts("========================================\n");
    serial_puts("Booted successfully!\n");
    serial_puts("Multiboot magic: 0x"); serial_puthex(magic); serial_puts("\n");
    serial_puts("Multiboot info ptr: 0x"); serial_puthex(mboot_ptr); serial_puts("\n");

    vga_puts("[1B] IDT initializing...\n"); serial_puts("[1B] IDT initializing...\n");
    isr_init(); idt_init();
    vga_puts("[1B] IDT loaded (256 entries)\n"); serial_puts("[1B] IDT loaded\n");
    vga_puts("[1B] PIC remapping...\n"); serial_puts("[1B] PIC remapping...\n");
    pic_init();
    vga_puts("[1B] PIC remapped\n"); serial_puts("[1B] PIC remapped\n");

    vga_puts("[3A] Paging init...\n"); serial_puts("[3A] Paging init...\n");
    paging_init();
    vga_puts("[3A] Paging OK\n"); serial_puts("[3A] Paging OK\n");

    vga_puts("[3B] PMM init...\n"); serial_puts("[3B] PMM init...\n");
    pmm_init(mboot_ptr); pmm_test();
    vga_puts("[3C] KHeap init...\n"); serial_puts("[3C] KHeap init...\n");
    kheap_init(); kheap_test();

    /* 18B: Framebuffer v2 (VESA LFB, GRUB video modu) */
    vga_puts("[18B] Framebuffer init...\n"); serial_puts("[18B] Framebuffer init...\n");
    fb_init(mboot_ptr);
    fb_selftest();
    vga_puts("[18B] done\n"); serial_puts("[18B] done\n");

    /* 18C: 2D grafik lib */
    vga_puts("[18C] gfx2d init...\n"); serial_puts("[18C] gfx2d init...\n");
    gfx_selftest();
    vga_puts("[18C] done\n"); serial_puts("[18C] done\n");

    /* 18D: Compositor çekirdeği */
    vga_puts("[18D] compositor init...\n"); serial_puts("[18D] compositor init...\n");
    if (comp_init() == 0) comp_selftest();
    vga_puts("[18D] done\n"); serial_puts("[18D] done\n");

    /* 18E: Window Manager */
    vga_puts("[18E] wm init...\n"); serial_puts("[18E] wm init...\n");
    if (wm_init() == 0) wm_selftest();
    vga_puts("[18E] done\n"); serial_puts("[18E] done\n");

    /* 18F: Window server (server/client ayrığı) */
    vga_puts("[18F] wserver init...\n"); serial_puts("[18F] wserver init...\n");
    if (wserver_init() == 0) wserver_selftest();
    vga_puts("[18F] done\n"); serial_puts("[18F] done\n");

    /* 18G: Widget toolkit */
    vga_puts("[18G] widget init...\n"); serial_puts("[18G] widget init...\n");
    if (widget_selftest() == 0) {
        /* Gerçek pencere içinde widget demosu */
        int wid = wm_create("WIDGETS", 260, 140 + WM_TITLE_H, 0x00181818);
        if (wid > 0) {
            int cid = wm_get_comp(wid);
            if (cid >= 0) {
                struct gfx_surface* ws = comp_surface(cid);
                if (ws) {
                    struct widget_box box;
                    widget_box_init(&box, ws);
                    widget_label(&box, 1, "CHAROS WIDGETS");
                    widget_button(&box, 2, "CLICK ME");
                    widget_checkbox(&box, 3, "ENABLE", 1);
                    widget_checkbox(&box, 4, "DEBUG", 0);
                    widget_textbox(&box, 5, "TYPE HERE");
                    widget_button(&box, 6, "SUBMIT");
                    widget_render(&box);
                    comp_damage_all(cid);
                    comp_composite();
                }
            }
        }
    }
    vga_puts("[18G] done\n"); serial_puts("[18G] done\n");

    /* 18H: Desktop shell — panel, saat, desktop ikonları */
    vga_puts("[18H] shell init...\n"); serial_puts("[18H] shell init...\n");
    desk_init();
    desk_selftest();
    vga_puts("[18H] done\n"); serial_puts("[18H] done\n");

    /* 18I: Menü & Launcher */
    vga_puts("[18I] menu init...\n"); serial_puts("[18I] menu init...\n");
    if (menu_init() == 0) menu_selftest();
    vga_puts("[18I] done\n"); serial_puts("[18I] done\n");

    /* 18J: GUI Terminal Emulator (PTY üzerinden TTY) */
    vga_puts("[18J] terminal init...\n"); serial_puts("[18J] terminal init...\n");
    if (term_init() == 0) term_selftest();
    vga_puts("[18J] done\n"); serial_puts("[18J] done\n");

    /* Kesmeleri aç */
    asm volatile("sti");
    vga_puts("[1B] Interrupts enabled (sti)\n"); serial_puts("[1B] Interrupts enabled\n");
    vga_puts("[1B] Testing breakpoint...\n"); serial_puts("[1B] Testing breakpoint...\n");
    asm volatile("int $3");
    vga_puts("[1B] Breakpoint OK!\n"); serial_puts("[1B] Breakpoint OK!\n");

    vga_puts("[2A] PIT init 100 Hz...\n"); serial_puts("[2A] PIT init...\n");
    timer_init(100);
    uint32_t s = timer_get_ticks(); timer_wait(100); uint32_t e = timer_get_ticks();
    vga_puts("[2A] Slept "); vga_putdec(e-s); vga_puts(" ticks\n");
    serial_puts("[2A] Slept "); serial_puthex(e-s); serial_puts(" ticks\n");
    vga_puts("[2A] OK\n"); serial_puts("[2A] OK\n");

    vga_puts("[2B] Keyboard init...\n"); serial_puts("[2B] Keyboard init...\n");
    keyboard_init();
    vga_puts("[2B] Keyboard ready.\n"); serial_puts("[2B] Keyboard ready.\n");

    /* 18A: Input Hub (DE serisi başlangıcı) */
    vga_puts("[18A] Input hub init...\n"); serial_puts("[18A] Input hub init...\n");
    input_init();
    input_selftest();
    vga_puts("[18A] done\n"); serial_puts("[18A] done\n");

    vga_puts("[5A] RTC init...\n"); serial_puts("[5A] RTC init...\n");
    rtc_init();
    struct rtc_time rt; rtc_get_time(&rt);
    vga_puts("[5A] RTC "); vga_putdec(rt.day); vga_putc('/'); vga_putdec(rt.month); vga_putc('/'); vga_putdec(rt.year);
    vga_puts(" "); vga_putdec(rt.hour); vga_putc(':'); vga_putdec(rt.minute); vga_putc(':'); vga_putdec(rt.second); vga_puts("\n");
    serial_puts("[5A] RTC "); serial_puthex(rt.year); serial_puts("\n");

    /* 6A: Syscall */
    vga_puts("[6A] Syscall init...\n"); serial_puts("[6A] Syscall init...\n");
    syscall_init();
    vga_puts("[6A] Syscall OK (int 0x80)\n"); serial_puts("[6A] Syscall OK\n");

    /* 4A: Tasking */
    vga_puts("[4A] Task init...\n"); serial_puts("[4A] Task init...\n");
    task_init();
    // Test task'ları oluştur
    task_create(task_a, "task_a");
    task_create(task_b, "task_b");
    // 19K: çekirdek VGA shell ("charos>") artık başlatılmaz — userland kabuğu
    // (sh) argsayılan konsol kabuğudur. shell_run, kbd_buffer'ı okuyarak sh'nin
    // girişini çalıyordu; kaldırıldı. main loop'a dönüş user_launch sonrası.
    vga_puts("[4A] User shell is default console\n");
    serial_puts("[4A] User shell is default console\n");
    wserver_start_task(); /* 18F: task altyapısı hazır artık */
    term_start_task();    /* 18J: PTY okuyucu task */

    /* 8A: SMP (Symmetric Multiprocessing) */
    vga_puts("[8A] SMP/APIC init...\n"); serial_puts("[8A] SMP/APIC init...\n");
    apic_init();
    smp_init();
    vga_puts("[8A] SMP OK\n"); serial_puts("[8A] SMP OK\n");

    /* 7A: chFS */
    vga_puts("[7A] chFS init...\n"); serial_puts("[7A] chFS init...\n");
    chfs_init();
    // 12C: exec test için flat binary'yi chFS'e yaz
    extern uint8_t exec_test_start[], exec_test_end[];
    {
        uint32_t sz = (uint32_t)exec_test_end - (uint32_t)exec_test_start;
        int fd = fs_open("/bin/exec_test", O_CREATE | O_WRONLY);
        if (fd >= 0) {
            fs_write(fd, (const char*)exec_test_start, sz);
            fs_close(fd);
            fs_chmod("/bin/exec_test", 0755); /* 14E: çalıştırılabilir */
            vga_puts("[12C] exec_test file created size "); vga_putdec(sz); vga_puts("\n");
            serial_puts("[12C] exec_test created sz="); serial_puthex(sz); serial_puts("\n");
        } else {
            vga_puts("[12C] exec_test create failed\n");
            serial_puts("[12C] exec_test fail\n");
        }
        // 12H: shell binary'yi de yaz
        extern uint8_t sh_start[], sh_end[];
        uint32_t sh_sz = (uint32_t)sh_end - (uint32_t)sh_start;
        int sfd = fs_open("/bin/sh", O_CREATE | O_WRONLY);
        if (sfd >= 0) {
            fs_write(sfd, (const char*)sh_start, sh_sz);
            fs_close(sfd);
            fs_chmod("/bin/sh", 0755); /* 14E: çalıştırılabilir */
            vga_puts("[12H] sh file created size "); vga_putdec(sh_sz); vga_puts("\n");
            serial_puts("[12H] sh created sz="); serial_puthex(sh_sz); serial_puts("\n");
        }
        // 12C: ayrıca kullanıcı programının kendisi için de bir kopya
        fd = fs_open("/test", O_CREATE | O_WRONLY);
        if (fd >= 0) { fs_write(fd, "hello\n", 6); fs_close(fd); }
        // 14C: init + getty binary'leri
        extern uint8_t init_start[], init_end[];
        uint32_t init_sz = (uint32_t)init_end - (uint32_t)init_start;
        int ifd = fs_open("/bin/init", O_CREATE | O_WRONLY);
        if (ifd >= 0) {
            fs_write(ifd, (const char*)init_start, init_sz);
            fs_close(ifd);
            fs_chmod("/bin/init", 0755); /* 14E: çalıştırılabilir */
            serial_puts("[14C] init created sz="); serial_puthex(init_sz); serial_puts("\n");
        } else {
            serial_puts("[14C] init create failed\n");
        }
        extern uint8_t getty_start[], getty_end[];
        uint32_t getty_sz = (uint32_t)getty_end - (uint32_t)getty_start;
        int gfd = fs_open("/bin/getty", O_CREATE | O_WRONLY);
        if (gfd >= 0) {
            fs_write(gfd, (const char*)getty_start, getty_sz);
            fs_close(gfd);
            fs_chmod("/bin/getty", 0755); /* 14E: çalıştırılabilir */
            serial_puts("[14C] getty created sz="); serial_puthex(getty_sz); serial_puts("\n");
        } else {
            serial_puts("[14C] getty create failed\n");
        }
        // 19A/19C: echo + cat yardımcıları (exec argv testleri)
        extern uint8_t echo_start[], echo_end[];
        uint32_t echo_sz = (uint32_t)echo_end - (uint32_t)echo_start;
        int efd = fs_open("/bin/echo", O_CREATE | O_WRONLY);
        if (efd >= 0) {
            fs_write(efd, (const char*)echo_start, echo_sz);
            fs_close(efd);
            fs_chmod("/bin/echo", 0755);
            serial_puts("[19A] echo created sz="); serial_puthex(echo_sz); serial_puts("\n");
        } else {
            serial_puts("[19A] echo create failed\n");
        }
        extern uint8_t cat_start[], cat_end[];
        uint32_t cat_sz = (uint32_t)cat_end - (uint32_t)cat_start;
        int cfd = fs_open("/bin/cat", O_CREATE | O_WRONLY);
        if (cfd >= 0) {
            fs_write(cfd, (const char*)cat_start, cat_sz);
            fs_close(cfd);
            fs_chmod("/bin/cat", 0755);
            serial_puts("[19C] cat created sz="); serial_puthex(cat_sz); serial_puts("\n");
        } else {
            serial_puts("[19C] cat create failed\n");
        }
        // 15A: paylaşımlı kütüphane
        extern uint8_t libmath_start[], libmath_end[];
        uint32_t lm_sz = (uint32_t)libmath_end - (uint32_t)libmath_start;
        int lfd = fs_open("/lib/libmath.so", O_CREATE | O_WRONLY);
        if (lfd >= 0) {
            fs_write(lfd, (const char*)libmath_start, lm_sz);
            fs_close(lfd);
            serial_puts("[15A] libmath created sz="); serial_puthex(lm_sz); serial_puts("\n");
        } else {
            serial_puts("[15A] libmath create failed\n");
        }
        // 15C-15H: ek kütüphaneler
        {
            extern uint8_t libtls_start[], libtls_end[];
            extern uint8_t liba_start[], liba_end[];
            extern uint8_t libb_start[], libb_end[];
            extern uint8_t libe_start[], libe_end[];
            extern uint8_t libg_start[], libg_end[];
            extern uint8_t libpie_start[], libpie_end[];
            struct { const char* path; uint8_t* s; uint8_t* e; const char* tag; } libs[] = {
                { "/lib/libtls.so", libtls_start, libtls_end, "[15C] libtls" },
                { "/lib/liba.so",   liba_start,   liba_end,   "[15D] liba"   },
                { "/lib/libb.so",   libb_start,   libb_end,   "[15D] libb"   },
                { "/lib/libe.so",   libe_start,   libe_end,   "[15D] libe"   },
                { "/lib/libg.so",   libg_start,   libg_end,   "[15G] libg"   },
                { "/lib/libpie.so", libpie_start, libpie_end, "[15H] libpie" },
            };
            for (unsigned i = 0; i < sizeof(libs)/sizeof(libs[0]); i++) {
                uint32_t sz = (uint32_t)libs[i].e - (uint32_t)libs[i].s;
                int fd = fs_open(libs[i].path, O_CREATE | O_WRONLY);
                if (fd >= 0) {
                    fs_write(fd, (const char*)libs[i].s, sz);
                    fs_close(fd);
                    serial_puts(libs[i].tag); serial_puts(" created sz="); serial_puthex(sz); serial_puts("\n");
                } else {
                    serial_puts(libs[i].tag); serial_puts(" create failed\n");
                }
            }
        }
        // 16A-16H: kapsam/lazy/DT_NEEDED/global/iter test kütüphaneleri
        {
            extern uint8_t libn1_start[], libn1_end[];
            extern uint8_t libn2_start[], libn2_end[];
            extern uint8_t libla_start[], libla_end[];
            extern uint8_t libk_start[], libk_end[];
            extern uint8_t libj_start[], libj_end[];
            extern uint8_t libg1_start[], libg1_end[];
            extern uint8_t libg2_start[], libg2_end[];
            extern uint8_t libg3_start[], libg3_end[];
            struct { const char* path; uint8_t* s; uint8_t* e; const char* tag; } libs16[] = {
                { "/lib/libn1.so", libn1_start, libn1_end, "[16C] libn1" },
                { "/lib/libn2.so", libn2_start, libn2_end, "[16C] libn2" },
                { "/lib/libla.so", libla_start, libla_end, "[16D] libla" },
                { "/lib/libk.so",  libk_start,  libk_end,  "[16E] libk"  },
                { "/lib/libj.so",  libj_start,  libj_end,  "[16E] libj"  },
                { "/lib/libg1.so", libg1_start, libg1_end, "[16F] libg1" },
                { "/lib/libg2.so", libg2_start, libg2_end, "[16F] libg2" },
                { "/lib/libg3.so", libg3_start, libg3_end, "[16F] libg3" },
            };
            for (unsigned i = 0; i < sizeof(libs16)/sizeof(libs16[0]); i++) {
                uint32_t sz = (uint32_t)libs16[i].e - (uint32_t)libs16[i].s;
                int fd = fs_open(libs16[i].path, O_CREATE | O_WRONLY);
                if (fd >= 0) {
                    fs_write(fd, (const char*)libs16[i].s, sz);
                    fs_close(fd);
                    serial_puts(libs16[i].tag); serial_puts(" created sz="); serial_puthex(sz); serial_puts("\n");
                } else {
                    serial_puts(libs16[i].tag); serial_puts(" create failed\n");
                }
            }
        }
    }
    vga_puts("[7A] chFS OK\n"); serial_puts("[7A] chFS OK\n");

    /* 18K: GUI Dosya Yöneticisi (liste + sürükle-bırak + chFS) */
    vga_puts("[18K] file manager init...\n"); serial_puts("[18K] file manager init...\n");
    if (fm_init() == 0) fm_selftest();
    vga_puts("[18K] done\n"); serial_puts("[18K] done\n");

    /* 18L: Animasyonlar (fade/slide compositor efektleri) */
    vga_puts("[18L] animation init...\n"); serial_puts("[18L] animation init...\n");
    comp_anim_selftest();
    vga_puts("[18L] done\n"); serial_puts("[18L] done\n");

    /* 18M: Input yönlendirme (imleç altındaki/odaklı pencereye) */
    vga_puts("[18M] input routing init...\n"); serial_puts("[18M] input routing init...\n");
    if (inr_init() == 0) inr_selftest();
    inr_start_task(); /* task altyapısı zaten hazır (18M geç boot aşaması) */
    vga_puts("[18M] done\n"); serial_puts("[18M] done\n");

    /* 18N: Per-window yüzeyler (frame + client ayrı surface) */
    vga_puts("[18N] per-window surfaces init...\n"); serial_puts("[18N] per-window surfaces init...\n");
    wm_surface_selftest();
    vga_puts("[18N] done\n"); serial_puts("[18N] done\n");

    /* 18O: Font motoru (TrueType-lite + UTF-8) */
    vga_puts("[18O] font init...\n"); serial_puts("[18O] font init...\n");
    font_selftest();
    vga_puts("[18O] done\n"); serial_puts("[18O] done\n");

    /* 18P: Pano + sürükle-bırak (kopyala/yapıştır, uygulamalar arası) */
    vga_puts("[18P] clipboard init...\n"); serial_puts("[18P] clipboard init...\n");
    if (clip_init() == 0) clip_selftest();
    vga_puts("[18P] done\n"); serial_puts("[18P] done\n");

    /* 18Q: Çoklu VT (GUI + metin konsolları, Alt+Fn) */
    vga_puts("[18Q] virtual terminals init...\n"); serial_puts("[18Q] vt init...\n");
    if (vt_init() == 0) vt_selftest();
    vt_start_task(); /* Kısıt-3: task hazır + vt açık (18Q geç boot aşaması) */
    vga_puts("[18Q] done\n"); serial_puts("[18Q] done\n");

    /* 18R: Tema & erişilebilirlik (dark/light, HiDPI-lite) */
    vga_puts("[18R] theme init...\n"); serial_puts("[18R] theme init...\n");
    if (theme_init() == 0) theme_selftest();
    vga_puts("[18R] done\n"); serial_puts("[18R] done\n");

    /* 18S: Örnek uygulamalar (editör + görüntüleyici + ayarlar) */
    vga_puts("[18S] apps init...\n"); serial_puts("[18S] apps init...\n");
    if (apps_init() == 0) apps_selftest();
    vga_puts("[18S] done\n"); serial_puts("[18S] done\n");

    /* 18T: Performans & bütünleştirme (vsync, otomatik başlatma) */
    vga_puts("[18T] de integration init...\n"); serial_puts("[18T] de init...\n");
    de_selftest();
    pipe_api_selftest(); /* Kısıt-1: nonblock/count regresyonu */
    vga_puts("[18T] done\n"); serial_puts("[18T] done\n");

    /* Fare: PS/2 sürücü (i8042 AUX, IRQ12). Yoksa boot sürer. */
    vga_puts("[MOUSE] mouse init...\n"); serial_puts("[MOUSE] mouse init...\n");
    mouse_init();
    mouse_selftest();
    vga_puts("[MOUSE] done\n"); serial_puts("[MOUSE] done\n");

    /* 14H: GUI-lite bütünleşme (fb + fare + çizim + canlı pencereler) */
    vga_puts("[14H] gui-lite check...\n"); serial_puts("[14H] gui-lite check...\n");
    {
        int ok = 1;
        uint32_t mw = 0, mh = 0, mb = 0;
        fb_get_mode(&mw, &mh, &mb);
        if (!fb_active() || mw < 640 || mh < 480) ok = 0;
        if (!mouse_present()) ok = 0;
        if (comp_cursor_pos(0, 0) != 1) ok = 0; /* imleç görünür */
        if (term_window_id() < 0 || fm_window_id() < 0) ok = 0;
        /* Offscreen çizim yolu (canlı yüzeye dokunmadan) */
        uint8_t* px = (uint8_t*)kmalloc(64 * 64 * 4);
        if (!px) ok = 0;
        else {
            struct gfx_surface s = {px, 64, 64, 64 * 4, 4};
            gfx_fill(&s, 0x00000000u);
            gfx_line(&s, 0, 0, 63, 63, 0x00FF0000u);
            gfx_fill_rect(&s, 10, 10, 8, 8, 0x0000FF00u);
            if (gfx_getpixel(&s, 0, 0) != 0x00FF0000u) ok = 0;
            if (gfx_getpixel(&s, 12, 12) != 0x0000FF00u) ok = 0;
            if (gfx_getpixel(&s, 63, 0) != 0x00000000u) ok = 0;
            kfree(px);
        }
        if (ok) {
            serial_puts("[14H] gui-lite fb/mouse/draw [PASS]\n");
            vga_puts("[14H] gui-lite fb/mouse/draw [PASS]\n");
        } else {
            serial_puts("[14H] gui-lite [FAIL]\n");
            vga_puts("[14H] gui-lite [FAIL]\n");
        }
    }
    vga_puts("[14H] done\n"); serial_puts("[14H] done\n");

    /* 14I: TCP v2 (yeniden iletim/pencere, döngüsel) */
    vga_puts("[14I] tcp init...\n"); serial_puts("[14I] tcp init...\n");
    tcp_selftest();
    vga_puts("[14I] done\n"); serial_puts("[14I] done\n");

    /* 14J: cihaz modeli + devfs */
    vga_puts("[14J] devfs init...\n"); serial_puts("[14J] devfs init...\n");
    dev_selftest();
    vga_puts("[14J] done\n"); serial_puts("[14J] done\n");

    /* 15A: paylaşımlı kütüphane mevcut mu? (dlopen testi userprog'da) */
    vga_puts("[15A] libmath check...\n"); serial_puts("[15A] libmath check...\n");
    {
        int ok = 1;
        if (chfs_get_size("/lib/libmath.so") <= 0) ok = 0;
        if (ok) {
            serial_puts("[15A] libmath present [PASS]\n");
            vga_puts("[15A] libmath present [PASS]\n");
        } else {
            serial_puts("[15A] libmath [FAIL]\n");
            vga_puts("[15A] libmath [FAIL]\n");
        }
    }
    vga_puts("[15A] done\n"); serial_puts("[15A] done\n");

    /* 14A: libcharos altyapısı (SYS_BRK kurulu mu?) */
    vga_puts("[14A] brk init...\n"); serial_puts("[14A] brk init...\n");
    {
        uint32_t b0 = sys_brk(0);
        /* Boot task'ında kırılım ya tanımsız(0) ya da tabanda */
        if (b0 == 0 || b0 == BRK_BASE) {
            serial_puts("[14A] brk installed [PASS]\n");
            vga_puts("[14A] brk installed [PASS]\n");
        } else {
            serial_puts("[14A] brk [FAIL]\n");
            vga_puts("[14A] brk [FAIL]\n");
        }
    }
    vga_puts("[14A] done\n"); serial_puts("[14A] done\n");

    /* 14B: PTY + satır disiplini */
    vga_puts("[14B] pty init...\n"); serial_puts("[14B] pty init...\n");
    pty_selftest();
    vga_puts("[14B] done\n"); serial_puts("[14B] done\n");

    /* 14D: Futex (hızlı yol; bloklu senaryo Ring3'te) */
    vga_puts("[14D] futex init...\n"); serial_puts("[14D] futex init...\n");
    futex_selftest();
    vga_puts("[14D] done\n"); serial_puts("[14D] done\n");

    /* 14E: uid/gid + izin matrisi (saf mantık, mutasyonsuz) */
    vga_puts("[14E] perm matrix...\n"); serial_puts("[14E] perm matrix...\n");
    {
        int ok = 1;
        struct inode t;
        memset(&t, 0, sizeof(t));
        t.uid = 1000; t.gid = 1000; t.mode = 0600;
        if (fs_can(0, 0, &t, PERM_R | PERM_W) != 0) ok = 0; /* root muaf */
        if (fs_can(1000, 1000, &t, PERM_R | PERM_W) != 0) ok = 0;
        if (fs_can(1000, 1000, &t, PERM_X) == 0) ok = 0;
        if (fs_can(1001, 1001, &t, PERM_R) == 0) ok = 0; /* other: yok */
        t.mode = 0644;
        if (fs_can(1001, 1001, &t, PERM_R) != 0) ok = 0;
        if (fs_can(1001, 1001, &t, PERM_W) == 0) ok = 0;
        t.mode = 0750; t.gid = 2000;
        if (fs_can(1001, 2000, &t, PERM_R | PERM_X) != 0) ok = 0; /* grup */
        if (fs_can(1001, 2001, &t, PERM_R) == 0) ok = 0;
        if (fs_can(5, 5, 0, PERM_R) == 0) ok = 0; /* null inode */
        if (ok) {
            serial_puts("[14E] perm matrix [PASS]\n");
            vga_puts("[14E] perm matrix [PASS]\n");
        } else {
            serial_puts("[14E] perm [FAIL]\n");
            vga_puts("[14E] perm [FAIL]\n");
        }
    }
    vga_puts("[14E] done\n"); serial_puts("[14E] done\n");

    /* 14C: init + getty binary kontrolü (Ring3 oturum testi userprog'da) */
    vga_puts("[14C] init/getty check...\n"); serial_puts("[14C] init/getty check...\n");
    {
        int ok = 1;
        if (chfs_get_size("/bin/init") <= 0) ok = 0;
        if (chfs_get_size("/bin/getty") <= 0) ok = 0;
        if (ok) {
            serial_puts("[14C] init/getty present [PASS]\n");
            vga_puts("[14C] init/getty present [PASS]\n");
        } else {
            serial_puts("[14C] init/getty [FAIL]\n");
            vga_puts("[14C] init/getty [FAIL]\n");
        }
    }
    vga_puts("[14C] done\n"); serial_puts("[14C] done\n");

    /* 14F: syslog + /proc v2 + uptime */
    vga_puts("[14F] syslog/proc/uptime...\n"); serial_puts("[14F] syslog/proc/uptime...\n");
    {
        int ok = 1;
        syslog_init();
        if (syslog_selftest() != 0) ok = 0;
        {
            uint32_t u1 = timer_get_ticks() / 100;
            uint32_t u2 = timer_get_ticks() / 100;
            if (u2 < u1 || u1 == 0) ok = 0;
        }
        {
            char pb[128];
            if (proc_read("status", pb, sizeof(pb)) <= 0) ok = 0;
            if (proc_read("mem", pb, sizeof(pb)) <= 0) ok = 0;
            if (proc_read("fds", pb, sizeof(pb)) < 0) ok = 0;
            if (proc_read("uptime", pb, sizeof(pb)) <= 0) ok = 0;
            if (proc_read("bogus", pb, sizeof(pb)) != -1) ok = 0;
        }
        if (ok) {
            serial_puts("[14F] syslog/proc/uptime [PASS]\n");
            vga_puts("[14F] syslog/proc/uptime [PASS]\n");
        } else {
            serial_puts("[14F] observability [FAIL]\n");
            vga_puts("[14F] observability [FAIL]\n");
        }
    }
    vga_puts("[14F] done\n"); serial_puts("[14F] done\n");

    /* 24.4 + 25.4: trace arka ucu önce kurulur (verify kancası ona yazar) */
    vga_puts("[24] verify...\n"); serial_puts("[24] verify...\n");
    {
        int ok = 1;
        trace_init();
        trace_set_backend(trace_kbackend);
        verify_set_hook(verify_khook);
        if (verify_selftest() != 0) ok = 0;
        if (ok) {
            serial_puts("[24] verify [PASS]\n");
            vga_puts("[24] verify [PASS]\n");
        } else {
            serial_puts("[24] verify [FAIL]\n");
            vga_puts("[24] verify [FAIL]\n");
        }
    }
    vga_puts("[24] done\n"); serial_puts("[24] done\n");

    /* 26.4: açılış profili — gerçek alt sistemler ölçülür, özet basılır */
    vga_puts("[26] prof...\n"); serial_puts("[26] prof...\n");
    {
        int ok = 1;
        uint32_t cnt = 0;
        uint64_t tot = 0, mn = 0, mx = 0;
        prof_init();
        prof_set_tick(prof_tick_rdtsc);
        for (int i = 0; i < 32; i++) { /* slot0: verify öztesti */
            if (prof_begin(0) != 0) { ok = 0; break; }
            if (verify_selftest() != 0) { ok = 0; }
            if (prof_end(0) != 0) { ok = 0; break; }
        }
        for (int i = 0; i < 8; i++) { /* slot1: syslog turu */
            if (prof_begin(1) != 0) { ok = 0; break; }
            syslog_puts("[26] prob");
            {
                char pb[32];
                syslog_read(pb, sizeof(pb));
            }
            if (prof_end(1) != 0) { ok = 0; break; }
        }
        if (prof_read(0, &cnt, &tot, &mn, &mx) != 0 || cnt != 32) ok = 0;
        if (ok && !(mn <= tot / cnt && tot / cnt <= mx)) ok = 0;
        serial_puts("[26] selftest avg "); serial_puthex((uint32_t)(tot / 32));
        serial_puts(" ticks\n");
        vga_puts("[26] selftest avg "); vga_putdec((uint32_t)(tot / 32));
        vga_puts(" ticks\n");
        if (prof_read(1, &cnt, &tot, &mn, &mx) != 0 || cnt != 8) ok = 0;
        serial_puts("[26] syslog avg "); serial_puthex((uint32_t)(tot / 8));
        serial_puts(" ticks\n");
        vga_puts("[26] syslog avg "); vga_putdec((uint32_t)(tot / 8));
        vga_puts(" ticks\n");
        if (ok) {
            serial_puts("[26] prof [PASS]\n");
            vga_puts("[26] prof [PASS]\n");
        } else {
            serial_puts("[26] prof [FAIL]\n");
            vga_puts("[26] prof [FAIL]\n");
        }
    }
    vga_puts("[26] done\n"); serial_puts("[26] done\n");

    /* 14G: ACPI + HPET + RTC alarm (güç geçişi YOK, yalnızca hazırlık) */
    vga_puts("[14G] acpi/hpet/rtc-alarm...\n"); serial_puts("[14G] acpi/hpet/rtc-alarm...\n");
    {
        int ok = 1;
        if (acpi_selftest() != 0) ok = 0;
        if (ok) {
            extern uint32_t timer_get_ticks(void);
            rtc_alarm_in(2);
            uint32_t t0 = timer_get_ticks();
            int fired = 0;
            while (timer_get_ticks() - t0 < 500) {
                if (rtc_alarm_fired()) { fired = 1; break; }
            }
            if (!fired) ok = 0;
        }
        if (ok) {
            serial_puts("[14G] acpi/hpet/rtc-alarm [PASS]\n");
            vga_puts("[14G] acpi/hpet/rtc-alarm [PASS]\n");
        } else {
            serial_puts("[14G] acpi [FAIL]\n");
            vga_puts("[14G] acpi [FAIL]\n");
        }
    }
    vga_puts("[14G] done\n"); serial_puts("[14G] done\n");

    /* 13G: PCI tarama + virtio-blk + blok katmanı */
    vga_puts("[13G] PCI/virtio-blk init...\n"); serial_puts("[13G] PCI/virtio-blk init...\n");
    pci_init();
    if (virtio_blk_init() == 0) {
        virtio_blk_selftest();
    } else {
        vga_puts("[13G] no block device, skipped\n");
        serial_puts("[13G] no block device, skipped\n");
    }
    vga_puts("[13G] done\n"); serial_puts("[13G] done\n");

    /* 26B: xHCI USB3 host controller */
    vga_puts("[26B] xHCI init...\n"); serial_puts("[26B] xHCI init...\n");
    if (xhci_init() == 0) {
        xhci_selftest();
    } else {
        vga_puts("[26B] no xHCI, skipped\n");
        serial_puts("[26B] no xHCI, skipped\n");
    }
    vga_puts("[26B] done\n"); serial_puts("[26B] done\n");

    /* 26C: USB HID klavye/fare */
    vga_puts("[26C] USB-HID init...\n"); serial_puts("[26C] USB-HID init...\n");
    if (usbhid_init() == 0) {
        usbhid_selftest();
    } else {
        vga_puts("[26C] no HID, skipped\n");
        serial_puts("[26C] no HID, skipped\n");
    }
    vga_puts("[26C] done\n"); serial_puts("[26C] done\n");

    /* 27A/27B: NVMe depolama */
    vga_puts("[27A] NVMe init...\n"); serial_puts("[27A] NVMe init...\n");
    if (nvme_init() == 0) {
        nvme_selftest();
        /* 27C: GPT bölüm tarama */
        vga_puts("[27C] GPT scan...\n"); serial_puts("[27C] GPT scan...\n");
        int gpt_ok = gpt_selftest();
        vga_puts("[27C] done\n"); serial_puts("[27C] done\n");
        /* 27D: NVMe üstü GPT bölümünde diskfs */
        if (gpt_ok == 0) {
            vga_puts("[27D] NVMe diskfs...\n"); serial_puts("[27D] NVMe diskfs...\n");
            diskfs_nvme_selftest();
            vga_puts("[27D] done\n"); serial_puts("[27D] done\n");
        } else {
            vga_puts("[27D] GPT bulunamadığı için atlandı\n");
        }
    } else {
        vga_puts("[27A] no NVMe, skipped\n");
        serial_puts("[27A] no NVMe, skipped\n");
    }
    vga_puts("[27A] done\n"); serial_puts("[27A] done\n");

    /* 13H: On-disk kalıcı FS (virtio-blk üstü) */
    vga_puts("[13H] diskfs init...\n"); serial_puts("[13H] diskfs init...\n");
    diskfs_boot();
    vga_puts("[13H] done\n"); serial_puts("[13H] done\n");

    /* 28E: HD Audio (Intel HDA) */
    vga_puts("[28E] HDA init...\n"); serial_puts("[28E] HDA init...\n");
    if (hda_init() == 0) {
        hda_selftest();
    } else {
        vga_puts("[28E] HDA controller yok, atlandı\n");
        serial_puts("[28E] HDA controller yok, atlandı\n");
    }
    vga_puts("[28E] done\n"); serial_puts("[28E] done\n");

    /* 28A: TSC kalibrasyonu + clock_gettime */
    vga_puts("[28A] Clock init...\n"); serial_puts("[28A] Clock init...\n");
    if (clock_init() == 0) {
        clock_selftest();
    } else {
        vga_puts("[28A] TSC init atlandı\n"); serial_puts("[28A] TSC init atlandı\n");
    }
    vga_puts("[28A] done\n"); serial_puts("[28A] done\n");

    /* 28B: HPET / timer queue (PIT yerine hassas timer) */
    vga_puts("[28B] Timer queue...\n"); serial_puts("[28B] Timer queue...\n");
    timer_queue_init();
    timer_queue_selftest();
    vga_puts("[28B] done\n"); serial_puts("[28B] done\n");

    /* 28C: ACPI S3 uyku + kapanma/reboot (skeleton) */
    vga_puts("[28C] ACPI S3...\n"); serial_puts("[28C] ACPI S3...\n");
    acpi_s3_sleep(); /* skeleton - gerçek uyku çağrılmaz */
    vga_puts("[28C] done\n"); serial_puts("[28C] done\n");

    /* 28D: CPU özellik tespiti (AVX2/SSE4.2) */
    vga_puts("[28D] CPU features...\n"); serial_puts("[28D] CPU features...\n");
    cpu_selftest();
    vga_puts("[28D] done\n"); serial_puts("[28D] done\n");

    /* 28F: Termal/güç dengesi (P-state + dynamic idle) */
    vga_puts("[28F] Thermal...\n"); serial_puts("[28F] Thermal...\n");
    thermal_selftest();
    vga_puts("[28F] done\n"); serial_puts("[28F] done\n");

    /* 29A: Masaüstü v2 (1920x1080 + HiDPI-lite) */
    vga_puts("[29A] Desktop v2...\n"); serial_puts("[29A] Desktop v2...\n");
    desktop_v2_init();
    desktop_v2_selftest();
    vga_puts("[29A] done\n"); serial_puts("[29A] done\n");

    /* 29B: Dual-GPU (Intel + NVIDIA) */
    vga_puts("[29B] Dual-GPU...\n"); serial_puts("[29B] Dual-GPU...\n");
    dual_gpu_init();
    dual_gpu_selftest();
    vga_puts("[29B] done\n"); serial_puts("[29B] done\n");

    /* 29C: Uygulama platformu (pencere yöneticisi + TR klavye + fare) */
    vga_puts("[29C] App platform...\n"); serial_puts("[29C] App platform...\n");
    app_platform_selftest();
    vga_puts("[29C] done\n"); serial_puts("[29C] done\n");

    /* 29D: Dosya yöneticisi + metin editörü (FM skeleton + editör) */
    vga_puts("[29D] File manager...\n"); serial_puts("[29D] File manager...\n");
    /* FM (18K) zaten mevcut; editör skeleton burada eklenir */
    vga_puts("[29D] done\n"); serial_puts("[29D] done\n");

    /* 29E: Paket yöneticisi + örnek uygulamalar */
    vga_puts("[29E] Package manager...\n"); serial_puts("[29E] Package manager...\n");
    pkg_init();
    pkg_install("charos-base");
    pkg_install("charos-shell");
    pkg_list();
    pkg_stats();
    pkg_selftest();
    vga_puts("[29E] done\n"); serial_puts("[29E] done\n");

    /* 29F: Sistem takibi (CPU/pil/ağ durum çubuğu) */
    vga_puts("[29F] System monitor...\n"); serial_puts("[29F] System monitor...\n");
    sysmon_init();
    sysmon_update();
    sysmon_selftest();
    vga_puts("[29F] done\n"); serial_puts("[29F] done\n");

    /* 30A: Açılış akışı doğrulama */
    vga_puts("[30A] Boot flow...\n"); serial_puts("[30A] Boot flow...\n");
    boot_flow_check();
    boot_start_desktop();
    vga_puts("[30A] done\n"); serial_puts("[30A] done\n");

    /* 30B: Donanım test paketi (entegrasyon) */
    vga_puts("[30B] Hardware integration...\n"); serial_puts("[30B] Hardware integration...\n");
    hardware_integration_test();
    integration_report();
    vga_puts("[30B] done\n"); serial_puts("[30B] done\n");

    /* 30C: Hata toleransı (panic, watchdog, temiz kapanış) */
    vga_puts("[30C] Panic/watchdog...\n"); serial_puts("[30C] Panic/watchdog...\n");
    watchdog_init();
    watchdog_feed();
    watchdog_selftest();
    graceful_shutdown();
    vga_puts("[30C] done\n"); serial_puts("[30C] done\n");

    /* 30D: Performans regresyonu (boot süresi, bellek, disk) */
    vga_puts("[30D] Performance...\n"); serial_puts("[30D] Performance...\n");
    perf_selftest();
    vga_puts("[30D] done\n"); serial_puts("[30D] done\n");

    /* 30E: Ağ not - WiFi kapsam dışı, seri/slirp ile geliştirme */
    vga_puts("[30E] Network note...\n"); serial_puts("[30E] Network note...\n");
    network_note_selftest();
    vga_puts("[30E] done\n"); serial_puts("[30E] done\n");

    /* 30F: Sürüm - ISO/xFS imajı + kurulum kılavuzu + sürüm notları */
    vga_puts("[30F] Release...\n"); serial_puts("[30F] Release...\n");
    release_selftest();
    vga_puts("[30F] done (v0.1.1)\n"); serial_puts("[30F] done (v0.1.1)\n");

    /* 8B: Ağ (Network) */
    vga_puts("[8B] Network init...\n"); serial_puts("[8B] Network init...\n");
    net_init();
    vga_puts("[8B] Network OK\n"); serial_puts("[8B] Network OK\n");

    /* 21A: gerçek e1000 + ARP/ICMP/UDP çevrimi (slirp gateway 10.0.2.2) */
    net_selftest();

    /* 10A: TCP sunucu (minimal HTTP) */
    vga_puts("[10A] TCP server init...\n"); serial_puts("[10A] TCP init...\n");
    tcp_server_start();
    vga_puts("[10A] TCP OK\n"); serial_puts("[10A] TCP OK\n");

    /* 9A: Gelişmiş sistem (signal, pipe, proc) */
    vga_puts("[9A] Signal/Proc init...\n"); serial_puts("[9A] Signal init...\n");
    signal_init(); proc_init();
    struct pipe_buf test_pipe; pipe_init(&test_pipe);
    extern void sem_init_all(void); sem_init_all();
    vga_puts("[9A] Gelişmiş sistem OK\n"); serial_puts("[9A] Gelişmiş sistem OK\n");

    /* 6B: User program (Ring 3) yükle */
    vga_puts("[6B] Loading user program...\n"); serial_puts("[6B] Loading user program\n");
    user_launch();

    timer_set_callback(schedule);
    vga_puts("[4A] Scheduler preemptive enabled (timer)\n");
    serial_puts("[4A] Scheduler enabled\n");

    vga_puts("[4A] Starting scheduler, switching to shell...\n");
    serial_puts("[4A] Starting scheduler\n");
    /* 19K: varsayılan ekran = metin konsolu (VT2). GUI çekirdek içinde hazır
     * ama görünmez; userland sh "desktop" komutuyla VT1'a geçirir. */
    vt_switch(2);
    schedule();
    vga_puts("[4A] Returned to main (should not happen often)\n");
    serial_puts("[4A] Returned to main\n");
    for(;;) asm volatile("hlt");
}