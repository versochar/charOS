# charOS Makefile - Using host GCC with -m32 for bare-metal i386
# Note: For production, use i686-elf- cross-compiler

CC      = gcc
LD      = ld
AS      = as
OBJCOPY = objcopy
OBJDUMP = objdump
NM      = nm

# Compiler flags for bare-metal 32-bit
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs
CFLAGS += -Iinclude
CFLAGS += -fno-stack-protector -fno-pic -fno-pie
CFLAGS += -m32 -march=i686
# Ek bulgu düzeltmesi: header bağımlılık takibi. .h değişince bağımlı
# .c'ler yeniden derlenir; bayat .o artığı kaynak gerçeğini maskeleyemez.
CFLAGS += -MMD -MP

# Assembler flags
ASFLAGS = --32

# Linker flags
LDFLAGS = -T linker.ld -nostdlib -melf_i386

# Directories
BOOT_DIR    = boot
KERNEL_DIR  = kernel
INCLUDE_DIR = include
BUILD_DIR   = build
ISO_DIR     = $(BUILD_DIR)/iso

# Source files
BOOT_SRCS   = $(BOOT_DIR)/boot.s \
              $(BOOT_DIR)/interrupts.s
KERNEL_SRCS = $(KERNEL_DIR)/kernel.c \
              $(KERNEL_DIR)/core/gdt.c \
              $(KERNEL_DIR)/core/idt.c \
              $(KERNEL_DIR)/core/pic.c \
              $(KERNEL_DIR)/core/isr.c \
              $(KERNEL_DIR)/core/spinlock.c \
              $(KERNEL_DIR)/core/syscall.c \
              $(KERNEL_DIR)/core/apic.c \
              $(KERNEL_DIR)/memory/paging.c \
              $(KERNEL_DIR)/memory/mmap.c \
              $(KERNEL_DIR)/memory/pmm.c \
              $(KERNEL_DIR)/memory/mb2.c \
              $(KERNEL_DIR)/memory/kheap.c \
              $(KERNEL_DIR)/memory/brk.c \
              $(KERNEL_DIR)/process/pty.c \
              $(KERNEL_DIR)/process/futex.c \
              $(KERNEL_DIR)/core/syslog.c \
              $(KERNEL_DIR)/drivers/vga.c \
              $(KERNEL_DIR)/drivers/serial.c \
              $(KERNEL_DIR)/drivers/timer.c \
              $(KERNEL_DIR)/drivers/keyboard.c \
              $(KERNEL_DIR)/drivers/mouse.c \
              $(KERNEL_DIR)/drivers/rtc.c \
              $(KERNEL_DIR)/drivers/acpi.c \
              $(KERNEL_DIR)/drivers/input.c \
              $(KERNEL_DIR)/drivers/fb.c \
              $(KERNEL_DIR)/drivers/gfx.c \
              $(KERNEL_DIR)/drivers/comp.c \
              $(KERNEL_DIR)/drivers/wm.c \
              $(KERNEL_DIR)/drivers/wserver.c \
              $(KERNEL_DIR)/drivers/widget.c \
              $(KERNEL_DIR)/drivers/shell.c \
              $(KERNEL_DIR)/drivers/menu.c \
              $(KERNEL_DIR)/drivers/term.c \
              $(KERNEL_DIR)/drivers/fm.c \
              $(KERNEL_DIR)/drivers/inr.c \
              $(KERNEL_DIR)/drivers/font.c \
              $(KERNEL_DIR)/drivers/clip.c \
              $(KERNEL_DIR)/drivers/vt.c \
              $(KERNEL_DIR)/drivers/theme.c \
              $(KERNEL_DIR)/drivers/apps.c \
              $(KERNEL_DIR)/drivers/de.c \
              $(KERNEL_DIR)/drivers/dev.c \
              $(KERNEL_DIR)/drivers/pci.c \
              $(KERNEL_DIR)/drivers/virtio_blk.c \
              $(KERNEL_DIR)/drivers/xhci.c \
              $(KERNEL_DIR)/drivers/usbhid.c \
              $(KERNEL_DIR)/drivers/nvme.c \
              $(KERNEL_DIR)/drivers/gpt.c \
              $(KERNEL_DIR)/drivers/blk.c \
              $(KERNEL_DIR)/drivers/hda.c \
              $(KERNEL_DIR)/drivers/clock.c \
              $(KERNEL_DIR)/drivers/timer_queue.c \
              $(KERNEL_DIR)/drivers/cpu_features.c \
              $(KERNEL_DIR)/drivers/thermal.c \
              $(KERNEL_DIR)/drivers/desktop_v2.c \
              $(KERNEL_DIR)/drivers/dual_gpu.c \
              $(KERNEL_DIR)/drivers/app_platform.c \
              $(KERNEL_DIR)/drivers/pkg_mgr.c \
              $(KERNEL_DIR)/drivers/sysmon.c \
              $(KERNEL_DIR)/core/boot_flow.c \
              $(KERNEL_DIR)/test/hardware.c \
              $(KERNEL_DIR)/drivers/panic.c \
              $(KERNEL_DIR)/drivers/perf.c \
              $(KERNEL_DIR)/drivers/network_note.c \
              $(KERNEL_DIR)/drivers/release.c \
              $(KERNEL_DIR)/lib/string.c \
               $(KERNEL_DIR)/process/shell.c \
               $(KERNEL_DIR)/process/task.c \
               $(KERNEL_DIR)/process/fork.c \
               $(KERNEL_DIR)/process/exec.c \
               $(KERNEL_DIR)/process/semaphore.c \
               $(KERNEL_DIR)/process/wait.c \
               $(KERNEL_DIR)/process/clone.c \
               $(KERNEL_DIR)/process/userprog.c \
               $(KERNEL_DIR)/fs/chfs.c \
              $(KERNEL_DIR)/net/net.c \
              $(KERNEL_DIR)/net/e1000.c \
              $(KERNEL_DIR)/net/tcp.c \
              $(KERNEL_DIR)/process/pipe.c \
              $(KERNEL_DIR)/process/signal.c \
              $(KERNEL_DIR)/fs/proc.c \
              $(KERNEL_DIR)/fs/diskfs.c
KERNEL_ASM_SRCS = $(KERNEL_DIR)/core/switch.s \
                  $(KERNEL_DIR)/embed_userprog.s \
                  $(KERNEL_DIR)/embed_exec_test.s \
                  $(KERNEL_DIR)/embed_sh.s \
                  $(KERNEL_DIR)/embed_init.s \
                  $(KERNEL_DIR)/embed_getty.s \
                  $(KERNEL_DIR)/embed_echo.s \
                  $(KERNEL_DIR)/embed_cat.s \
$(KERNEL_DIR)/embed_libmath.s \
                   $(KERNEL_DIR)/embed_libtls.s \
                   $(KERNEL_DIR)/embed_liba.s \
                   $(KERNEL_DIR)/embed_libb.s \
                   $(KERNEL_DIR)/embed_libe.s \
$(KERNEL_DIR)/embed_libg.s \
                    $(KERNEL_DIR)/embed_libpie.s \
                    $(KERNEL_DIR)/embed_libn1.s \
                    $(KERNEL_DIR)/embed_libn2.s \
                    $(KERNEL_DIR)/embed_libla.s \
                    $(KERNEL_DIR)/embed_libk.s \
                    $(KERNEL_DIR)/embed_libj.s \
                    $(KERNEL_DIR)/embed_libg1.s \
                    $(KERNEL_DIR)/embed_libg2.s \
                    $(KERNEL_DIR)/embed_libg3.s \
                    $(KERNEL_DIR)/embed_trampoline.s
KERNEL_ASM_OBJS = $(patsubst $(KERNEL_DIR)/%.s,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))

# Object files
BOOT_OBJS   = $(patsubst $(BOOT_DIR)/%.s,$(BUILD_DIR)/%.o,$(BOOT_SRCS))
KERNEL_OBJS = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ALL_OBJS    = $(BOOT_OBJS) $(KERNEL_OBJS) $(KERNEL_ASM_OBJS)

# Output files
KERNEL_ELF  = $(BUILD_DIR)/kernel.elf
KERNEL_BIN  = $(BUILD_DIR)/kernel.bin
ISO_FILE    = $(BUILD_DIR)/charos.iso

# User program (Ring 3, bladder as blob)
USERPROG_C      = user/userprog.c
USERPROG_ELF    = $(BUILD_DIR)/userprog.elf
USERPROG_BIN    = $(BUILD_DIR)/userprog.bin
EXEC_TEST_C     = user/exec_test.c
EXEC_TEST_ELF   = $(BUILD_DIR)/exec_test.elf
EXEC_TEST_BIN   = $(BUILD_DIR)/exec_test.bin
SH_C            = user/sh.c
SH_ELF          = $(BUILD_DIR)/sh.elf
SH_BIN          = $(BUILD_DIR)/sh.bin
INIT_C          = user/init.c
INIT_ELF        = $(BUILD_DIR)/init.elf
INIT_BIN        = $(BUILD_DIR)/init.bin
GETTY_C         = user/getty.c
GETTY_ELF       = $(BUILD_DIR)/getty.elf
GETTY_BIN       = $(BUILD_DIR)/getty.bin
ECHO_C          = user/echo.c
ECHO_ELF        = $(BUILD_DIR)/echo.elf
ECHO_BIN        = $(BUILD_DIR)/echo.bin
CAT_C           = user/cat.c
CAT_ELF         = $(BUILD_DIR)/cat.elf
CAT_BIN         = $(BUILD_DIR)/cat.bin

# Default target (test diski de hazır olur; varsa silinmez)
all: $(KERNEL_BIN) $(ISO_FILE) $(HD_IMG)

# 14A: libcharos ortak kitaplığı (userprog'a linklenir)
LIBCHAROS_C   = user/libcharos.c
LIBCHAROS_OBJ = $(BUILD_DIR)/libcharos.o
$(LIBCHAROS_OBJ): $(LIBCHAROS_C) user/libcharos.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(LIBCHAROS_C) -o $@

# 14I/15B: user yardımcıları (userprog'a linklenir)
USERHELP_C   = user/dns.c user/http.c user/pthread.c user/dl.c
USERHELP_OBJ = $(BUILD_DIR)/dns.o $(BUILD_DIR)/http.o $(BUILD_DIR)/pthread.o $(BUILD_DIR)/dl.o
$(BUILD_DIR)/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# User program: freestanding, link 16MB, raw binary
# 19A-notu: userprog.c -O0 ile derlenir. -O2, syscall-yoğun bu dosyada
# yanlış makine kodu üretiyordu (yanlış syscall numarası/operant karışması);
# test kütüğü için -O0 yapısal olarak güvenlidir (argümanlar stack'te).
$(USERPROG_BIN): $(USERPROG_C) $(LIBCHAROS_OBJ) $(USERHELP_OBJ) user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $(USERPROG_C) -o $(BUILD_DIR)/userprog.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(USERPROG_ELF) $(BUILD_DIR)/userprog.o $(LIBCHAROS_OBJ) $(USERHELP_OBJ)
	$(OBJCOPY) -O binary --strip-all $(USERPROG_ELF) $@
	@echo "User program built: $@"

$(EXEC_TEST_BIN): $(EXEC_TEST_C) user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(EXEC_TEST_C) -o $(BUILD_DIR)/exec_test.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(EXEC_TEST_ELF) $(BUILD_DIR)/exec_test.o
	$(OBJCOPY) -O binary --strip-all $(EXEC_TEST_ELF) $@
	@echo "Exec test program built: $@"

$(SH_BIN): $(SH_C) user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -O0 -c $(SH_C) -o $(BUILD_DIR)/sh.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(SH_ELF) $(BUILD_DIR)/sh.o
	$(OBJCOPY) -O binary --strip-all $(SH_ELF) $@
	@echo "Shell built: $@ (size $$(stat -c%s $@) bytes, limit 32768; -O0: syscall-heavy kodda -O2 yanlış derliyor)"

$(INIT_BIN): $(INIT_C) user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(INIT_C) -o $(BUILD_DIR)/init.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(INIT_ELF) $(BUILD_DIR)/init.o
	$(OBJCOPY) -O binary --strip-all $(INIT_ELF) $@
	@echo "Init built: $@"

$(GETTY_BIN): $(GETTY_C) user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(GETTY_C) -o $(BUILD_DIR)/getty.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(GETTY_ELF) $(BUILD_DIR)/getty.o
	$(OBJCOPY) -O binary --strip-all $(GETTY_ELF) $@
	@echo "Getty built: $@"

# 19A/19C: echo/cat yardımcıları (exec argv konvansiyonu: user/args.h)
$(ECHO_BIN): $(ECHO_C) user/args.h user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(ECHO_C) -o $(BUILD_DIR)/echo.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(ECHO_ELF) $(BUILD_DIR)/echo.o
	$(OBJCOPY) -O binary --strip-all $(ECHO_ELF) $@
	@echo "Echo built: $@"

$(CAT_BIN): $(CAT_C) user/args.h user/user.ld
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(CAT_C) -o $(BUILD_DIR)/cat.o
	$(LD) -T user/user.ld -nostdlib -melf_i386 -o $(CAT_ELF) $(BUILD_DIR)/cat.o
	$(OBJCOPY) -O binary --strip-all $(CAT_ELF) $@
	@echo "Cat built: $@"

# 15A: paylaşımlı kütüphane (ET_DYN, sysv-hash, eager bind)
LIBMATH_C   = user/libmath.c
LIBMATH_SO  = $(BUILD_DIR)/libmath.so
# Not: chfs dosya sınırı 16KB (CHFS_DATABLOCKS) -> sıkı link (sayfa dolgusu yok, strip).
$(LIBMATH_SO): $(LIBMATH_C)
	@mkdir -p $(dir $@)
	$(CC) -std=gnu99 -ffreestanding -Os -Wall -m32 -march=i686 -Iinclude -fno-stack-protector -fno-asynchronous-unwind-tables -nostdlib -shared -fPIC -Wl,--hash-style=sysv -Wl,-soname,libmath.so -Wl,-z,now -Wl,-z,norelro -Wl,-z,noseparate-code -Wl,--build-id=none -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

# 15C-15H: ek paylaşımlı kütüphaneler (aynı sıkı kurallar)
SHLIB_FLAGS = -std=gnu99 -ffreestanding -Os -Wall -m32 -march=i686 -Iinclude -fno-stack-protector -fno-asynchronous-unwind-tables -nostdlib -shared -fPIC -Wl,--hash-style=sysv -Wl,-z,now -Wl,-z,norelro -Wl,-z,noseparate-code -Wl,--build-id=none

LIBTLS_C   = user/libtls.c
LIBTLS_SO  = $(BUILD_DIR)/libtls.so
$(LIBTLS_SO): $(LIBTLS_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -ftls-model=global-dynamic -Wl,-soname,libtls.so -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

LIBA_C   = user/liba.c
LIBA_SO  = $(BUILD_DIR)/liba.so
$(LIBA_SO): $(LIBA_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,liba.so -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

LIBB_C   = user/libb.c
LIBB_SO  = $(BUILD_DIR)/libb.so
$(LIBB_SO): $(LIBB_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,libb.so -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

LIBE_C   = user/libe.c
LIBE_SO  = $(BUILD_DIR)/libe.so
$(LIBE_SO): $(LIBE_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,libe.so -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

LIBG_C   = user/libg.c
LIBG_SO  = $(BUILD_DIR)/libg.so
$(LIBG_SO): $(LIBG_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,libg.so -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

LIBPIE_C   = user/libpie.c
LIBPIE_SO  = $(BUILD_DIR)/libpie.so
$(LIBPIE_SO): $(LIBPIE_C)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,libpie.so -Wl,-e,pie_run -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

# embed_userprog.s, userprog.bin'e bağlı (incbin)
$(BUILD_DIR)/embed_userprog.o: $(KERNEL_DIR)/embed_userprog.s $(USERPROG_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_exec_test.o: $(KERNEL_DIR)/embed_exec_test.s $(EXEC_TEST_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# embed_sh.s, sh.bin'e bağlı (incbin) - eksik kural temiz derlemeyi patlatıyordu
$(BUILD_DIR)/embed_sh.o: $(KERNEL_DIR)/embed_sh.s $(SH_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_init.o: $(KERNEL_DIR)/embed_init.s $(INIT_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_getty.o: $(KERNEL_DIR)/embed_getty.s $(GETTY_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# 19A/19C: echo/cat gömülü binary'leri
$(BUILD_DIR)/embed_echo.o: $(KERNEL_DIR)/embed_echo.s $(ECHO_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_cat.o: $(KERNEL_DIR)/embed_cat.s $(CAT_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libmath.o: $(KERNEL_DIR)/embed_libmath.s $(LIBMATH_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libtls.o: $(KERNEL_DIR)/embed_libtls.s $(LIBTLS_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_liba.o: $(KERNEL_DIR)/embed_liba.s $(LIBA_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libb.o: $(KERNEL_DIR)/embed_libb.s $(LIBB_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libe.o: $(KERNEL_DIR)/embed_libe.s $(LIBE_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libg.o: $(KERNEL_DIR)/embed_libg.s $(LIBG_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/embed_libpie.o: $(KERNEL_DIR)/embed_libpie.s $(LIBPIE_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# 16A-16H: kapsam (RTLD_NEXT/DEFAULT), lazy PLT, DT_NEEDED, RTLD_GLOBAL,
# dl_iterate_phdr ve ana program tutamacı için test kütüphaneleri.
define SHLIB_RULE
$(1)_C  = user/$(2).c
$(1)_SO = $$(BUILD_DIR)/$(2).so
$$($(1)_SO): $$($(1)_C)
	@mkdir -p $$(dir $$@)
	$$(CC) $$(SHLIB_FLAGS) -Wl,-soname,$(2).so -o $$@ $$<
	strip --strip-unneeded $$@
	@echo "Shared lib built: $$@"
endef

$(eval $(call SHLIB_RULE,LIBN1,libn1))
$(eval $(call SHLIB_RULE,LIBN2,libn2))
$(eval $(call SHLIB_RULE,LIBLA,libla))
$(eval $(call SHLIB_RULE,LIBK,libk))
$(eval $(call SHLIB_RULE,LIBG1,libg1))
$(eval $(call SHLIB_RULE,LIBG2,libg2))
$(eval $(call SHLIB_RULE,LIBG3,libg3))

# 16E: libj, libk'ya bağımlı (DT_NEEDED) — link sırasında libk.so gerekir.
LIBJ_C  = user/libj.c
LIBJ_SO = $(BUILD_DIR)/libj.so
$(LIBJ_SO): $(LIBJ_C) $(LIBK_SO)
	@mkdir -p $(dir $@)
	$(CC) $(SHLIB_FLAGS) -Wl,-soname,libj.so -L$(BUILD_DIR) -lk -o $@ $<
	strip --strip-unneeded $@
	@echo "Shared lib built: $@"

# 16: embed kuralları (incbin -> gömülü kütüphaneler)
$(BUILD_DIR)/embed_libn1.o: $(KERNEL_DIR)/embed_libn1.s $(LIBN1_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libn2.o: $(KERNEL_DIR)/embed_libn2.s $(LIBN2_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libla.o: $(KERNEL_DIR)/embed_libla.s $(LIBLA_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libk.o: $(KERNEL_DIR)/embed_libk.s $(LIBK_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libj.o: $(KERNEL_DIR)/embed_libj.s $(LIBJ_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libg1.o: $(KERNEL_DIR)/embed_libg1.s $(LIBG1_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libg2.o: $(KERNEL_DIR)/embed_libg2.s $(LIBG2_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/embed_libg3.o: $(KERNEL_DIR)/embed_libg3.s $(LIBG3_SO)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# AP trampoline: ayrı link 0x8000 (VMA), raw binary olarak gömülür (11D)
TRAMPOLINE_BIN = $(BUILD_DIR)/trampoline.bin
$(TRAMPOLINE_BIN): $(BUILD_DIR)/trampoline.o
	@mkdir -p $(dir $@)
	$(LD) -Ttext 0x8000 -melf_i386 --oformat binary -o $@ $<
	@echo "Trampoline binary: $@"

$(BUILD_DIR)/embed_trampoline.o: $(KERNEL_DIR)/embed_trampoline.s $(TRAMPOLINE_BIN)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Build rules
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ALL_OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(ALL_OBJS)
	cp $(KERNEL_ELF) $@
	@echo "Kernel built: $@ (ELF)"
	@$(NM) $(KERNEL_ELF) | grep -E '(_start|kernel_end)' || true
	@grub-file --is-x86-multiboot $(KERNEL_ELF) && echo "Multiboot header: OK" || echo "Multiboot header: FAILED"

# ISO creation
$(ISO_FILE): $(KERNEL_BIN) grub.cfg
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null
	@echo "ISO created: $@"

# 26A: UEFI bootloader (BOOTX64.EFI) + FAT imaj (gerçek makine/OVMF)
UEFI_DIR     = boot/uefi
UEFI_BUILD   = $(BUILD_DIR)/uefi
UEFI_CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -m64 -mabi=ms \
               -mno-red-zone -fno-stack-protector -fno-pic \
               -nostdlib -nostartfiles -nodefaultlibs -I$(UEFI_DIR)
UEFI_EFI     = $(UEFI_BUILD)/BOOTX64.EFI
UEFI_IMG     = $(BUILD_DIR)/uefi.img

$(UEFI_BUILD)/loader.o: $(UEFI_DIR)/loader.c $(UEFI_DIR)/uefi.h
	@mkdir -p $(UEFI_BUILD)
	$(CC) $(UEFI_CFLAGS) -c $< -o $@

$(UEFI_BUILD)/tramp.o: $(UEFI_DIR)/tramp.S
	@mkdir -p $(UEFI_BUILD)
	$(CC) -m64 -c $< -o $@

# Doğrudan PE32+ link (i386pep): .reloc + subsystem=10, OVMF taşıyabilir
# (link-pe.ld: gereksiz bölümler atılır; VMA=0 .comment OVMF'yi bozuyordu)
$(UEFI_EFI): $(UEFI_BUILD)/loader.o $(UEFI_BUILD)/tramp.o
	$(LD) -m i386pep -T $(UEFI_DIR)/link-pe.ld --subsystem 10 --image-base 0 --nxcompat --dynamicbase -e efi_main -o $@ $^
	@grub-file --is-x86_64-efi $@ && echo "EFI binary: OK" || echo "EFI binary: FAILED"

# FAT imaj: MBR + FAT32 bölümü: /EFI/BOOT/BOOTX64.EFI + /boot/kernel.bin
# (OVMF HD boot, USB'ye dd'lenir)
$(UEFI_IMG): $(UEFI_EFI) $(KERNEL_BIN)
	@rm -f $@
	dd if=/dev/zero of=$@ bs=1M count=64 status=none
	parted -s $@ mklabel msdos mkpart primary fat32 1MiB 100% set 1 boot on set 1 esp on
	@OFF=$$((2048*512)); \
	mkfs.vfat -F 32 -n CHAROS --offset 2048 $@ >/dev/null; \
	mmd -i $@@@$$OFF ::/EFI ::/EFI/BOOT ::/boot; \
	mcopy -i $@@@$$OFF $< ::/EFI/BOOT/BOOTX64.EFI; \
	mcopy -i $@@@$$OFF $(KERNEL_BIN) ::/boot/kernel.bin
	@echo "UEFI image: $@"

UEFI_EFIBOOT  = $(UEFI_BUILD)/efiboot.img
UEFI_CD      = $(BUILD_DIR)/charos-uefi.iso

# El Torito EFI boot imajı (grub-mkrescue ile aynı: FAT12 2880K): loader + kernel
# 26A notu: mtools MUTLAKA C locale ile (tr_TR'de dizin girdileri bozuluyor!)
$(UEFI_EFIBOOT): $(UEFI_EFI) $(KERNEL_BIN)
	@rm -f $@
	dd if=/dev/zero of=$@ bs=1024 count=2880 status=none
	export LC_ALL=C; \
	mformat -i $@ -t 80 -h 2 -s 36 ::; \
	mmd -i $@ ::/EFI; \
	mmd -i $@ ::/EFI/BOOT; \
	mmd -i $@ ::/boot; \
	mcopy -i $@ $(UEFI_EFI) ::/EFI/BOOT/BOOTX64.EFI; \
	mcopy -i $@ $(KERNEL_BIN) ::/boot/kernel.bin
	@echo "EFIBOOT image: $@"

# UEFI CD (OVMF -cdrom ile boot; GRUB'suz direkt loader)
$(UEFI_CD): $(UEFI_EFIBOOT)
	xorriso -as mkisofs -o $@ -e efiboot.img -no-emul-boot \
		-V CHAROS_UEFI $(UEFI_BUILD) 2>/dev/null
	@echo "UEFI CD: $@"

uefi: $(UEFI_IMG)

uefi-cd: $(UEFI_CD)

uefi: $(UEFI_IMG)

# UEFI boot testi (OVMF, IDE uefi.img): ~60sn, seri loga bak
# 26A: -cpu max (x2APIC testi için), e1000 + virtio-blk test diski
run-uefi: $(UEFI_CD) $(HD_IMG)
	@rm -f uefi.log
	@timeout 120 qemu-system-x86_64 -no-reboot -cpu max -bios /usr/share/edk2/x64/OVMF.4m.fd -cdrom $(UEFI_CD) -smp 2 -m 128M -device e1000 $(QEMU_DRIVE) -serial file:uefi.log -display none 2>/dev/null; true
	@echo "PASS: $$(grep -c '\[PASS\]' uefi.log)"
	@grep '\[FAIL\]' uefi.log || echo "FAIL: none"

# 27A: UEFI + USB-HID + NVMe tam testi (~200sn)
run-uefi-nvme: $(UEFI_CD) $(HD_IMG) $(NVME_IMG)
	@rm -f uefi-nvme.log
	@timeout 200 qemu-system-x86_64 -no-reboot -cpu max -bios /usr/share/edk2/x64/OVMF.4m.fd -cdrom $(UEFI_CD) -smp 2 -m 512M -device qemu-xhci -device usb-kbd -device usb-mouse -device e1000 $(QEMU_DRIVE) $(QEMU_NVME) -serial file:uefi-nvme.log -display none 2>/dev/null; true
	@echo "PASS: $$(grep -c '\[PASS\]' uefi-nvme.log)"
	@grep '\[FAIL\]' uefi-nvme.log || echo "FAIL: none"

# 13G: virtio-blk test diski (32MB, BİR KEZ oluşur; kökte durur,
# make clean yalnızca build/'i siler, bu disk korunur -> kalıcılık için)
HD_IMG = hd.img
# 27A: NVMe test diski (64MB). QEMU legacy `drive=` tek NS (nsid 1) kurar.
# 27C: sfdisk GPT damgası (LBA2048'den charOS bölümü; yedek GPT sondadır).
# 27B selftest son-100 LBA'yı kullanır (yedek GPT'yi ezmez).
NVME_IMG = nvme.img
CHAROS_GUID = A1B2C3D4-E5F6-4A7B-8C9D-E0F1A2B3C4D5
$(NVME_IMG):
	@if [ ! -f $@ ]; then qemu-img create -f raw $@ 64M; printf 'label: gpt\nstart=2048, type=$(CHAROS_GUID), name="charOS"\n' | sfdisk $@; fi
# GPT damgasını yenile (mevcut imajı SİLER + yeniden kurar)
nvme-gpt:
	@rm -f $(NVME_IMG)
	@$(MAKE) $(NVME_IMG)
QEMU_NVME = -drive file=$(NVME_IMG),format=raw,if=none,id=nvm -device nvme,serial=deadbeef,drive=nvm
# 21B: x2APIC, default qemu32 CPU'da kapalıdır; -cpu max ile açılır.
QEMU_CPU = max
QEMU_CFLAGS = -cpu $(QEMU_CPU)
$(HD_IMG):
	@if [ ! -f $@ ]; then qemu-img create -f raw $@ 32M; fi

# Tam test paketi (headless, diskli): ~220sn koşar, PASS/FAIL özeti basar
test-full: $(ISO_FILE) $(HD_IMG)
	@rm -f /tmp/charos_test.log
	@timeout 300 qemu-system-i386 $(QEMU_CFLAGS) -cdrom $(ISO_FILE) -smp 2 -m 128M -device e1000 $(QEMU_DRIVE) -serial file:/tmp/charos_test.log -display none >/dev/null 2>&1; true
	@echo "PASS: $$(grep -c '\[PASS\]' /tmp/charos_test.log)"
	@grep '\[FAIL\]' /tmp/charos_test.log || echo "FAIL: none"

# Run in QEMU - direkt çalıştır (spec ile aynı)
# QEMU default display'i kullanır (Arch'ta gtk/sdl, container'da none)
# Pencere açılır ve kernel banner'da halt'ta kalır - kapatana kadar bekler (donma değil)
QEMU_DRIVE = -drive file=$(HD_IMG),format=raw,if=none,id=hd0 -device virtio-blk-pci-transitional,drive=hd0
run-iso: $(ISO_FILE) $(HD_IMG)
	qemu-system-i386 $(QEMU_CFLAGS) -cdrom $(ISO_FILE) -smp 2 -m 128M -device e1000 $(QEMU_DRIVE) -serial file:serial.log

# Headless test (CI) - otomatik 6sn sonra kapat ve serial.log göster (klavye timeout için)
run-iso-headless: $(ISO_FILE) $(HD_IMG)
	@rm -f serial.log
	@timeout 7 qemu-system-i386 $(QEMU_CFLAGS) -cdrom $(ISO_FILE) -smp 2 -m 128M -device e1000 $(QEMU_DRIVE) -serial file:serial.log -display none 2>&1 & pid=$$!; sleep 6; kill $$pid 2>/dev/null; wait $$pid 2>/dev/null; echo "=== serial.log ==="; cat serial.log || echo "(empty)"

# Direct kernel boot test (without GRUB)
run-kernel: $(KERNEL_ELF)
	@rm -f serial.log
	@timeout 4 qemu-system-i386 -kernel $(KERNEL_ELF) -smp 2 -m 128M -serial file:serial.log -display none 2>&1 & pid=$$!; sleep 3; kill $$pid 2>/dev/null; wait $$pid 2>/dev/null; echo "=== serial.log ==="; cat serial.log || echo "(empty)"

# Debug with GDB
debug: $(ISO_FILE) $(HD_IMG)
	qemu-system-i386 -cdrom $(ISO_FILE) -smp 2 -m 128M -device e1000 $(QEMU_DRIVE) -serial file:serial.log -s -S

# Üretilen header bağımlılıkları (.d): ilk derlemede yoktur, sessiz geçilir.
-include $(KERNEL_OBJS:.o=.d)
-include $(BUILD_DIR)/userprog.d $(BUILD_DIR)/exec_test.d $(BUILD_DIR)/sh.d

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Kaynaktan tam doğrulamalı derleme (bayat artifakt şüphesinde kullan)
rebuild: clean all

# 32A-32J: 64-bit bellek agaci derleme kontrolu (32-bit kurallara dokunmaz)
K64_DIR = kernel/arch/x86_64
K64_SRCS = $(K64_DIR)/paging64.c \
           $(K64_DIR)/longmode.c \
           $(K64_DIR)/kernel64.c \
           $(K64_DIR)/idt64.c \
           $(K64_DIR)/gdt64.c \
           $(K64_DIR)/apic64.c \
           $(K64_DIR)/protect64.c \
           $(K64_DIR)/pmm64.c \
           $(K64_DIR)/slab64.c \
           $(K64_DIR)/demand64.c \
           $(K64_DIR)/cow64.c \
           $(K64_DIR)/kaslr64.c \
           $(K64_DIR)/percpu_pt.c \
           $(K64_DIR)/huge64.c \
           $(K64_DIR)/swap64.c \
           $(K64_DIR)/pressure64.c \
           $(K64_DIR)/smp64.c \
           $(K64_DIR)/aptramp64.c \
           $(K64_DIR)/lapic_timer64.c \
           $(K64_DIR)/tlb64.c \
           $(K64_DIR)/workqueue64.c \
           $(K64_DIR)/affinity64.c \
           $(K64_DIR)/numa64.c \
           $(K64_DIR)/rcu64.c \
           $(K64_DIR)/lfalloc64.c \
           $(K64_DIR)/acpi64.c \
           $(K64_DIR)/gop64.c \
           $(K64_DIR)/nvram64.c \
           $(K64_DIR)/shim64.c \
           $(K64_DIR)/runtime64.c \
           $(K64_DIR)/var64.c \
           $(K64_DIR)/zone64.c \
           $(K64_DIR)/buddy64.c \
           $(K64_DIR)/reclaim64.c \
           $(K64_DIR)/oom64.c \
           $(K64_DIR)/notify64.c \
           $(K64_DIR)/balloon64.c \
           $(K64_DIR)/ksm64.c \
           $(K64_DIR)/hugeswap64.c \
           $(K64_DIR)/memtest64.c \
           $(K64_DIR)/cachecolor64.c \
           $(K64_DIR)/profile64.c \
           $(K64_DIR)/leak64.c \
           $(K64_DIR)/kasan64.c \
           $(K64_DIR)/guard64.c \
           $(K64_DIR)/align64.c \
           $(K64_DIR)/arena64.c \
           $(K64_DIR)/jem64.c \
           $(K64_DIR)/aslr64.c \
           $(K64_DIR)/nx64.c \
           $(K64_DIR)/scanary64.c \
           $(K64_DIR)/seccomp64.c \
           $(K64_DIR)/capaudit64.c \
           $(K64_DIR)/selinux64.c \
           $(K64_DIR)/apparmor64.c \
           $(K64_DIR)/userns64.c \
           $(K64_DIR)/modsign64.c \
           $(K64_DIR)/passwd64.c \
           $(K64_DIR)/pam64.c \
           $(K64_DIR)/pammod64.c \
           $(K64_DIR)/sudo64.c \
           $(K64_DIR)/capv364.c \
           $(K64_DIR)/auditlog64.c \
           $(K64_DIR)/group64.c \
           $(K64_DIR)/selinuxctx64.c \
           $(K64_DIR)/privdrop64.c \
           $(K64_DIR)/vfsops64.c \
           $(K64_DIR)/dentry64.c \
           $(K64_DIR)/mountns64.c \
           $(K64_DIR)/symhard64.c \
           $(K64_DIR)/flock64.c \
           $(K64_DIR)/inotify64.c \
           $(K64_DIR)/poll64.c \
           $(K64_DIR)/ioctl64.c \
           $(K64_DIR)/tmpfs64.c \
           $(K64_DIR)/ext2_64.c \
           $(K64_DIR)/journal64.c \
           $(K64_DIR)/fsync64.c \
           $(K64_DIR)/xattr64.c \
           $(K64_DIR)/acl64.c \
           $(K64_DIR)/quota64.c \
           $(K64_DIR)/fsck64.c \
           $(K64_DIR)/snap64.c \
           $(K64_DIR)/fuse64.c \
           $(K64_DIR)/charpkg64.c \
           $(K64_DIR)/repo64.c \
           $(K64_DIR)/depsolve64.c \
           $(K64_DIR)/sign64.c \
           $(K64_DIR)/txn64.c \
           $(K64_DIR)/upgrade64.c \
           $(K64_DIR)/mirror64.c \
           $(K64_DIR)/flatpak64.c \
           $(K64_DIR)/manifest64.c \
           $(K64_DIR)/ipstack64.c \
           $(K64_DIR)/arpndp64.c \
           $(K64_DIR)/cubic64.c \
           $(K64_DIR)/udp64.c \
           $(K64_DIR)/dhcp64.c \
           $(K64_DIR)/dns64.c \
           $(K64_DIR)/tls64.c \
           $(K64_DIR)/slaac64.c \
           $(K64_DIR)/netfilter64.c \
           $(K64_DIR)/uas64.c \
           $(K64_DIR)/usbaudio64.c \
           $(K64_DIR)/uvc64.c \
           $(K64_DIR)/hubpower64.c \
           $(K64_DIR)/msi64.c \
           $(K64_DIR)/usbip64.c \
           $(K64_DIR)/hidparse64.c \
           $(K64_DIR)/virthid64.c \
           $(K64_DIR)/usbdevfs64.c \
           $(K64_DIR)/hdacodec64.c \
           $(K64_DIR)/alsa64.c \
           $(K64_DIR)/mixer64.c \
           $(K64_DIR)/pulse64.c \
           $(K64_DIR)/jack64.c \
           $(K64_DIR)/btaudio64.c \
           $(K64_DIR)/src64.c \
           $(K64_DIR)/lowlat64.c \
           $(K64_DIR)/sndtest64.c \
           $(K64_DIR)/media64.c \
           $(K64_DIR)/prelink64.c \
           $(K64_DIR)/ldpath64.c \
           $(K64_DIR)/nss64.c \
           $(K64_DIR)/iconvprov64.c \
           $(K64_DIR)/cryptoprov64.c \
           $(K64_DIR)/plugin64.c \
           $(K64_DIR)/hotplug64.c \
           $(K64_DIR)/selftest64.c \
           $(K64_DIR)/linktest64.c \
           $(K64_DIR)/shell64.c \
           $(K64_DIR)/coreutils64.c \
           $(K64_DIR)/busybox64.c \
           $(K64_DIR)/textutils64.c \
           $(K64_DIR)/sshclient64.c \
           $(K64_DIR)/gitclient64.c \
           $(K64_DIR)/make64.c \
           $(K64_DIR)/python64.c \
           $(K64_DIR)/node64.c \
           $(K64_DIR)/wayland64.c \
           $(K64_DIR)/xcompat64.c \
           $(K64_DIR)/inputv264.c \
           $(K64_DIR)/outputv264.c \
           $(K64_DIR)/virtkey64.c \
           $(K64_DIR)/screenshare64.c \
           $(K64_DIR)/remote64.c \
           $(K64_DIR)/secctx64.c \
           $(K64_DIR)/lowlatpath64.c \
           $(K64_DIR)/panel64.c \
           $(K64_DIR)/wmde64.c \
           $(K64_DIR)/settings64.c \
           $(K64_DIR)/theme64.c \
           $(K64_DIR)/fileman64.c \
           $(K64_DIR)/launcher64.c \
           $(K64_DIR)/notifyd64.c \
           $(K64_DIR)/taskbar64.c \
           $(K64_DIR)/ctxmenu64.c \
           $(K64_DIR)/elfparse64.c \
           $(K64_DIR)/progheader64.c \
           $(K64_DIR)/dynlink64.c \
           $(K64_DIR)/soload64.c \
           $(K64_DIR)/aslruser64.c \
           $(K64_DIR)/rpath64.c \
           $(K64_DIR)/delayload64.c \
           $(K64_DIR)/symver64.c \
           $(K64_DIR)/dlopen64.c \
           $(K64_DIR)/gallium64.c \
           $(K64_DIR)/kms64.c \
           $(K64_DIR)/vk64.c \
           $(K64_DIR)/gles64.c \
           $(K64_DIR)/vram64.c \
           $(K64_DIR)/glcomp64.c \
           $(K64_DIR)/sched64.c \
           $(K64_DIR)/compute64.c \
           $(K64_DIR)/vaapi64.c \
           $(K64_DIR)/btrfs64.c \
           $(K64_DIR)/zfs64.c \
           $(K64_DIR)/sshfs64.c \
           $(K64_DIR)/nfsclient64.c \
           $(K64_DIR)/nfsserver64.c \
           $(K64_DIR)/smb64.c \
           $(K64_DIR)/luks64.c \
           $(K64_DIR)/overlay64.c \
           $(K64_DIR)/encfs64.c \
           $(K64_DIR)/acpiparse64.c \
           $(K64_DIR)/s364.c \
           $(K64_DIR)/s464.c \
           $(K64_DIR)/cpufreq64.c \
           $(K64_DIR)/cpuidle64.c \
           $(K64_DIR)/powerbtn64.c \
           $(K64_DIR)/battery64.c \
           $(K64_DIR)/thermal64.c \
           $(K64_DIR)/wol64.c \
           $(K64_DIR)/hwmon64.c \
           $(K64_DIR)/cputemp64.c \
           $(K64_DIR)/fan64.c \
           $(K64_DIR)/battstat64.c \
           $(K64_DIR)/sysmon64.c \
           $(K64_DIR)/prom64.c \
           $(K64_DIR)/smart64.c \
           $(K64_DIR)/logrotate64.c \
           $(K64_DIR)/health64.c \
           $(K64_DIR)/musl64.c \
           $(K64_DIR)/pthread64.c \
           $(K64_DIR)/dnsresolv64.c \
           $(K64_DIR)/stdio64.c \
           $(K64_DIR)/locale64.c \
           $(K64_DIR)/tz64.c \
           $(K64_DIR)/iconv64.c \
           $(K64_DIR)/regex64.c \
           $(K64_DIR)/math64.c \
           $(K64_DIR)/rtlprobe64.c \
           $(K64_DIR)/fwload64.c \
           $(K64_DIR)/mac11_64.c \
           $(K64_DIR)/wpa364.c \
           $(K64_DIR)/scan64.c \
           $(K64_DIR)/powersave64.c \
           $(K64_DIR)/apmode64.c \
           $(K64_DIR)/mesh64.c \
           $(K64_DIR)/drvtest64.c \
           $(K64_DIR)/init64.c \
           $(K64_DIR)/unit64.c \
           $(K64_DIR)/sockact64.c \
           $(K64_DIR)/journald64.c \
           $(K64_DIR)/loginctl64.c \
           $(K64_DIR)/cgroup64.c \
           $(K64_DIR)/timer64.c \
           $(K64_DIR)/boottarget64.c \
           $(K64_DIR)/rescue64.c
K64_CFLAGS = -m64 -ffreestanding -O2 -Wall -Wextra -Iinclude \
             -iquote boot/uefi -fno-stack-protector -fno-pic -fno-pie
check64:
	@for f in $(K64_SRCS); do \
		$(CC) $(K64_CFLAGS) -c $$f -o /tmp/check64_$$(basename $$f .c).o || exit 1; \
	done
	@$(AS) --64 boot/x86_64/boot64.s -o /tmp/check64_boot64.o
	@$(AS) --64 boot/x86_64/aptramp64.s -o /tmp/check64_aptramp64.o
	@$(CC) -m64 -c boot/uefi/elf64.c -o /tmp/check64_elf64.o -Iboot/uefi
	@$(CC) -m64 -ffreestanding -O2 -Wall -Wextra -c boot/uefi/elf64load.c -o /tmp/check64_elf64load.o -Iboot/uefi
	@$(CC) -m64 -ffreestanding -O2 -Wall -Wextra -c boot/uefi/cleanup64.c -o /tmp/check64_cleanup64.o -Iboot/uefi
	@echo "check64: OK ($$(echo $(K64_SRCS) | wc -w) C + boot64.s + aptramp64.s + elf64.c + elf64load.c + cleanup64.c)"

# 32J: host bellek testi (-iquote sart; duz -I kernel string.h'i gölgeler)
test-mem64:
	$(CC) -iquote include $(K64_DIR)/pmm64.c $(K64_DIR)/slab64.c \
		$(K64_DIR)/demand64.c $(K64_DIR)/cow64.c $(K64_DIR)/kaslr64.c \
		$(K64_DIR)/percpu_pt.c $(K64_DIR)/huge64.c $(K64_DIR)/swap64.c \
		$(K64_DIR)/pressure64.c tests/host/test_mem64.c -o /tmp/test_mem64
	/tmp/test_mem64

# 33J: host SMP testi (AP blob linklenir, yalniz okunur)
test-smp64:
	$(AS) --64 boot/x86_64/aptramp64.s -o /tmp/aptramp64_test.o
	$(CC) -iquote include $(K64_DIR)/smp64.c $(K64_DIR)/aptramp64.c \
		$(K64_DIR)/lapic_timer64.c $(K64_DIR)/tlb64.c \
		$(K64_DIR)/workqueue64.c $(K64_DIR)/affinity64.c \
		$(K64_DIR)/numa64.c $(K64_DIR)/rcu64.c $(K64_DIR)/lfalloc64.c \
		tests/host/test_smp64.c /tmp/aptramp64_test.o -no-pie -o /tmp/test_smp64
	/tmp/test_smp64

# 34J: host UEFI testi
test-uefi64:
	$(CC) -iquote include -iquote boot/uefi $(K64_DIR)/acpi64.c \
		$(K64_DIR)/gop64.c $(K64_DIR)/nvram64.c $(K64_DIR)/shim64.c \
		$(K64_DIR)/runtime64.c $(K64_DIR)/var64.c boot/uefi/elf64.c \
		boot/uefi/elf64load.c boot/uefi/cleanup64.c \
		tests/host/test_uefi64.c -o /tmp/test_uefi64
	/tmp/test_uefi64

# 34H: 64-bit ISO UEFI boot dogrulama (El Torito icerigi + boyutlar)
iso64-check: $(UEFI_CD)
	export LC_ALL=C; \
	mdir -i $(UEFI_EFIBOOT) ::/EFI/BOOT/BOOTX64.EFI; \
	mdir -i $(UEFI_EFIBOOT) ::/boot/kernel.bin
	@echo "iso64-check: OK ($$(stat -c%s $(UEFI_CD)) bayt ISO)"

# 35J: host swap/PMM testi
test-swap64:
	$(CC) -iquote include $(K64_DIR)/pmm64.c $(K64_DIR)/swap64.c \
		$(K64_DIR)/cow64.c $(K64_DIR)/pressure64.c $(K64_DIR)/zone64.c \
		$(K64_DIR)/buddy64.c $(K64_DIR)/reclaim64.c $(K64_DIR)/oom64.c \
		$(K64_DIR)/notify64.c $(K64_DIR)/balloon64.c $(K64_DIR)/ksm64.c \
		$(K64_DIR)/hugeswap64.c $(K64_DIR)/memtest64.c \
		tests/host/test_swap64.c -o /tmp/test_swap64
	/tmp/test_swap64

# 36J: host heap testi
test-heap64:
	$(CC) -iquote include $(K64_DIR)/pmm64.c $(K64_DIR)/slab64.c \
		$(K64_DIR)/cachecolor64.c $(K64_DIR)/profile64.c \
		$(K64_DIR)/leak64.c $(K64_DIR)/kasan64.c $(K64_DIR)/guard64.c \
		$(K64_DIR)/align64.c $(K64_DIR)/arena64.c $(K64_DIR)/jem64.c \
		tests/host/test_heap64.c -o /tmp/test_heap64
	/tmp/test_heap64

# 37J: host guvenlik testi
test-sec64:
	$(CC) -iquote include $(K64_DIR)/aslr64.c $(K64_DIR)/nx64.c \
		$(K64_DIR)/scanary64.c $(K64_DIR)/seccomp64.c \
		$(K64_DIR)/capaudit64.c $(K64_DIR)/selinux64.c \
		$(K64_DIR)/apparmor64.c $(K64_DIR)/userns64.c \
		$(K64_DIR)/modsign64.c tests/host/test_sec64.c -o /tmp/test_sec64
	/tmp/test_sec64

# 38J: host yetki testi
test-auth64:
	$(CC) -iquote include $(K64_DIR)/passwd64.c $(K64_DIR)/pam64.c \
		$(K64_DIR)/pammod64.c $(K64_DIR)/sudo64.c $(K64_DIR)/capv364.c \
		$(K64_DIR)/auditlog64.c $(K64_DIR)/group64.c \
		$(K64_DIR)/selinuxctx64.c $(K64_DIR)/privdrop64.c \
		tests/host/test_auth64.c -o /tmp/test_auth64
	/tmp/test_auth64

# 39J: host VFS testi
test-vfs64:
	$(CC) -iquote include $(K64_DIR)/pmm64.c $(K64_DIR)/vfsops64.c \
		$(K64_DIR)/dentry64.c $(K64_DIR)/mountns64.c \
		$(K64_DIR)/symhard64.c $(K64_DIR)/flock64.c \
		$(K64_DIR)/inotify64.c $(K64_DIR)/poll64.c $(K64_DIR)/ioctl64.c \
		$(K64_DIR)/tmpfs64.c tests/host/test_vfs64.c -o /tmp/test_vfs64
	/tmp/test_vfs64

# 40J: host FS testi
test-fs64:
	$(CC) -iquote include $(K64_DIR)/ext2_64.c $(K64_DIR)/journal64.c \
		$(K64_DIR)/fsync64.c $(K64_DIR)/xattr64.c $(K64_DIR)/acl64.c \
		$(K64_DIR)/quota64.c $(K64_DIR)/fsck64.c $(K64_DIR)/snap64.c \
		$(K64_DIR)/fuse64.c tests/host/test_fs64.c -o /tmp/test_fs64
	/tmp/test_fs64

# 41J: host paket testi
test-pkg64:
	$(CC) -iquote include $(K64_DIR)/charpkg64.c $(K64_DIR)/repo64.c \
		$(K64_DIR)/depsolve64.c $(K64_DIR)/sign64.c $(K64_DIR)/txn64.c \
		$(K64_DIR)/upgrade64.c $(K64_DIR)/mirror64.c \
		$(K64_DIR)/flatpak64.c $(K64_DIR)/manifest64.c \
		tests/host/test_pkg64.c -o /tmp/test_pkg64
	/tmp/test_pkg64

# 42J: host init testi
test-init64:
	$(CC) -iquote include $(K64_DIR)/init64.c $(K64_DIR)/unit64.c \
		$(K64_DIR)/sockact64.c $(K64_DIR)/journald64.c \
		$(K64_DIR)/loginctl64.c $(K64_DIR)/cgroup64.c \
		$(K64_DIR)/timer64.c $(K64_DIR)/boottarget64.c \
		$(K64_DIR)/rescue64.c tests/host/test_init64.c -o /tmp/test_init64
	/tmp/test_init64

# 43J: host ag testi
test-net64:
	$(CC) -iquote include $(K64_DIR)/ipstack64.c $(K64_DIR)/arpndp64.c \
		$(K64_DIR)/cubic64.c $(K64_DIR)/udp64.c $(K64_DIR)/dhcp64.c \
		$(K64_DIR)/dns64.c $(K64_DIR)/tls64.c $(K64_DIR)/slaac64.c \
		$(K64_DIR)/netfilter64.c tests/host/test_net64.c -o /tmp/test_net64
	/tmp/test_net64

# 44J: host WiFi testi
test-wifi64:
	$(CC) -iquote include $(K64_DIR)/rtlprobe64.c $(K64_DIR)/fwload64.c \
		$(K64_DIR)/mac11_64.c $(K64_DIR)/wpa364.c $(K64_DIR)/scan64.c \
		$(K64_DIR)/powersave64.c $(K64_DIR)/apmode64.c \
		$(K64_DIR)/mesh64.c $(K64_DIR)/drvtest64.c \
		tests/host/test_wifi64.c -o /tmp/test_wifi64
	/tmp/test_wifi64

# 45J: host USB testi
test-usb64:
	$(CC) -iquote include $(K64_DIR)/uas64.c $(K64_DIR)/usbaudio64.c \
		$(K64_DIR)/uvc64.c $(K64_DIR)/hubpower64.c $(K64_DIR)/msi64.c \
		$(K64_DIR)/usbip64.c $(K64_DIR)/hidparse64.c \
		$(K64_DIR)/virthid64.c $(K64_DIR)/usbdevfs64.c \
		tests/host/test_usb64.c -o /tmp/test_usb64
	/tmp/test_usb64

# 46J: host ses testi
test-audio64:
	$(CC) -iquote include $(K64_DIR)/hdacodec64.c $(K64_DIR)/alsa64.c \
		$(K64_DIR)/mixer64.c $(K64_DIR)/pulse64.c $(K64_DIR)/jack64.c \
		$(K64_DIR)/btaudio64.c $(K64_DIR)/src64.c $(K64_DIR)/lowlat64.c \
		$(K64_DIR)/sndtest64.c $(K64_DIR)/media64.c \
		tests/host/test_audio64.c -o /tmp/test_audio64
	/tmp/test_audio64

# 47J: host GPU testi
test-gpu64:
	$(CC) -iquote include $(K64_DIR)/gallium64.c $(K64_DIR)/kms64.c \
		$(K64_DIR)/vk64.c $(K64_DIR)/gles64.c $(K64_DIR)/vram64.c \
		$(K64_DIR)/glcomp64.c $(K64_DIR)/sched64.c $(K64_DIR)/compute64.c \
		$(K64_DIR)/vaapi64.c tests/host/test_gpu64.c -o /tmp/test_gpu64
	/tmp/test_gpu64

# 48J: host guc testi
test-power64:
	$(CC) -iquote include $(K64_DIR)/acpi64.c $(K64_DIR)/acpiparse64.c \
		$(K64_DIR)/s364.c $(K64_DIR)/s464.c $(K64_DIR)/cpufreq64.c \
		$(K64_DIR)/cpuidle64.c $(K64_DIR)/powerbtn64.c \
		$(K64_DIR)/battery64.c $(K64_DIR)/thermal64.c $(K64_DIR)/wol64.c \
		tests/host/test_power64.c -o /tmp/test_power64
	/tmp/test_power64

# 49J: host sensor testi
test-sensor64:
	$(CC) -iquote include $(K64_DIR)/hwmon64.c $(K64_DIR)/cputemp64.c \
		$(K64_DIR)/fan64.c $(K64_DIR)/battstat64.c $(K64_DIR)/sysmon64.c \
		$(K64_DIR)/prom64.c $(K64_DIR)/smart64.c $(K64_DIR)/logrotate64.c \
		$(K64_DIR)/health64.c tests/host/test_sensor64.c \
		-o /tmp/test_sensor64
	/tmp/test_sensor64

# 50J: host gelismis-FS testi
test-fsadv64:
	$(CC) -iquote include $(K64_DIR)/btrfs64.c $(K64_DIR)/zfs64.c \
		$(K64_DIR)/sshfs64.c $(K64_DIR)/nfsclient64.c \
		$(K64_DIR)/nfsserver64.c $(K64_DIR)/smb64.c $(K64_DIR)/luks64.c \
		$(K64_DIR)/overlay64.c $(K64_DIR)/encfs64.c \
		tests/host/test_fsadv64.c -o /tmp/test_fsadv64
	/tmp/test_fsadv64

# 51J: host libc testi (dnsresolv64 -> dns64/ipstack64 bagimliligi)
test-libc64:
	$(CC) -iquote include $(K64_DIR)/musl64.c $(K64_DIR)/pthread64.c \
		$(K64_DIR)/dnsresolv64.c $(K64_DIR)/dns64.c \
		$(K64_DIR)/ipstack64.c $(K64_DIR)/stdio64.c \
		$(K64_DIR)/locale64.c $(K64_DIR)/tz64.c $(K64_DIR)/iconv64.c \
		$(K64_DIR)/regex64.c $(K64_DIR)/math64.c \
		tests/host/test_libc64.c -o /tmp/test_libc64
	/tmp/test_libc64

# 52J: host ELF testi (aslruser->aslr/kaslr, symver->repo bagimliligi)
test-elf64:
	$(CC) -iquote include -iquote boot/uefi $(K64_DIR)/elfparse64.c \
		$(K64_DIR)/progheader64.c $(K64_DIR)/dynlink64.c \
		$(K64_DIR)/soload64.c $(K64_DIR)/aslr64.c $(K64_DIR)/kaslr64.c \
		$(K64_DIR)/aslruser64.c $(K64_DIR)/rpath64.c \
		$(K64_DIR)/delayload64.c $(K64_DIR)/repo64.c \
		$(K64_DIR)/symver64.c $(K64_DIR)/dlopen64.c \
		boot/uefi/elf64.c tests/host/test_elf64.c -o /tmp/test_elf64
	/tmp/test_elf64

# 53J: host baglanti testi (linktest->delayload bagimliligi)
test-link64:
	$(CC) -iquote include $(K64_DIR)/prelink64.c $(K64_DIR)/ldpath64.c \
		$(K64_DIR)/rpath64.c $(K64_DIR)/nss64.c \
		$(K64_DIR)/iconvprov64.c $(K64_DIR)/cryptoprov64.c \
		$(K64_DIR)/plugin64.c $(K64_DIR)/hotplug64.c \
		$(K64_DIR)/selftest64.c $(K64_DIR)/delayload64.c \
		$(K64_DIR)/linktest64.c tests/host/test_link64.c \
		-o /tmp/test_link64
	/tmp/test_link64

# 54J: host kullanici-alani testi (textutils->regex bagimliligi)
test-user64:
	$(CC) -iquote include $(K64_DIR)/shell64.c $(K64_DIR)/coreutils64.c \
		$(K64_DIR)/busybox64.c $(K64_DIR)/regex64.c \
		$(K64_DIR)/textutils64.c $(K64_DIR)/sshclient64.c \
		$(K64_DIR)/gitclient64.c $(K64_DIR)/make64.c \
		$(K64_DIR)/python64.c $(K64_DIR)/node64.c \
		tests/host/test_user64.c -o /tmp/test_user64
	/tmp/test_user64

# 55J: host pencere-sunucusu testi
test-wserver64:
	$(CC) -iquote include $(K64_DIR)/wayland64.c $(K64_DIR)/xcompat64.c \
		$(K64_DIR)/inputv264.c $(K64_DIR)/outputv264.c \
		$(K64_DIR)/virtkey64.c $(K64_DIR)/screenshare64.c \
		$(K64_DIR)/remote64.c $(K64_DIR)/secctx64.c \
		$(K64_DIR)/lowlatpath64.c tests/host/test_wserver64.c \
		-o /tmp/test_wserver64
	/tmp/test_wserver64

# 56J: host DE testi
test-de64:
	$(CC) -iquote include $(K64_DIR)/panel64.c $(K64_DIR)/wmde64.c \
		$(K64_DIR)/settings64.c $(K64_DIR)/theme64.c \
		$(K64_DIR)/fileman64.c $(K64_DIR)/launcher64.c \
		$(K64_DIR)/notifyd64.c $(K64_DIR)/taskbar64.c \
		$(K64_DIR)/ctxmenu64.c $(K64_DIR)/gamed64.c $(K64_DIR)/controller64.c $(K64_DIR)/fps64.c $(K64_DIR)/perfhud64.c $(K64_DIR)/replay64.c $(K64_DIR)/achievement64.c $(K64_DIR)/mod64.c $(K64_DIR)/cloudsave64.c $(K64_DIR)/anticheat64.c $(K64_DIR)/userns64.c $(K64_DIR)/seccomp64.c $(K64_DIR)/mountns64.c $(K64_DIR)/pidns64.c $(K64_DIR)/netns64.c $(K64_DIR)/cgroup64.c $(K64_DIR)/apparmor64.c $(K64_DIR)/flatpak64.c $(K64_DIR)/runtime64.c $(K64_DIR)/liveiso64.c $(K64_DIR)/installer64.c $(K64_DIR)/partition64.c $(K64_DIR)/luks64.c $(K64_DIR)/bootloader64.c $(K64_DIR)/netinstall64.c $(K64_DIR)/pxe64.c $(K64_DIR)/autoinstall64.c $(K64_DIR)/recovery64.c $(K64_DIR)/release64.c $(K64_DIR)/changelog64.c $(K64_DIR)/manpages64.c $(K64_DIR)/apidocs64.c $(K64_DIR)/website64.c $(K64_DIR)/cicd64.c $(K64_DIR)/paketdepo64.c $(K64_DIR)/security64.c $(K64_DIR)/lts64.c $(K64_DIR)/releasev1_64.c $(K64_DIR)/secureboot64.c $(K64_DIR)/tpm64.c $(K64_DIR)/advmem64.c $(K64_DIR)/proc64.c $(K64_DIR)/sched64.c $(K64_DIR)/ipc64.c $(K64_DIR)/sync64.c $(K64_DIR)/drvhal64.c $(K64_DIR)/pcihal64.c $(K64_DIR)/blk64.c $(K64_DIR)/part64.c $(K64_DIR)/vfs64.c $(K64_DIR)/jrnl64.c $(K64_DIR)/fscrypt64.c $(K64_DIR)/virt64.c tests/host/test_de64.c -o /tmp/test_de64
	/tmp/test_de64

# Phony targets
.PHONY: all clean rebuild run-iso run-iso-headless run-kernel debug check64 test-mem64 test-smp64 test-uefi64 iso64-check test-swap64 test-heap64 test-sec64 test-auth64 test-vfs64 test-fs64 test-pkg64 test-init64 test-net64 test-wifi64 test-usb64 test-audio64 test-gpu64 test-power64 test-sensor64 test-fsadv64 test-libc64 test-elf64 test-link64 test-user64 test-wserver64 test-de64