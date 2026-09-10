/* 31C-31I: x86_64 long mode ortak header (freestanding, -m64) */
#ifndef CHAROS_LONGMODE_H
#define CHAROS_LONGMODE_H

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

/* --- 31C: paging64 --- */
#define PML4_ENTRIES 512
#define PDP_ENTRIES 512
#define PD_ENTRIES 512
#define PAGE_PRESENT64  (1ULL << 0)
#define PAGE_RW64       (1ULL << 1)
#define PAGE_US64       (1ULL << 2)
#define PAGE_PWT64      (1ULL << 3)
#define PAGE_PCD64      (1ULL << 4)
#define PAGE_PS64       (1ULL << 7)   /* 2MiB huge */
#define PAGE_NX64       (1ULL << 63)

/* 2MB cap, 512MB VRAM + 2GB RAM hedefi icin yeterli identity map */
void paging64_init(u64 *pml4);
void paging64_map_2mb(u64 *pml4, u64 virt, u64 phys, u64 flags);
int paging64_is_longmode(void);

/* --- 31D: longmode activation --- */
void longmode_enable_paging(u64 pml4_phys);
u64 longmode_get_cr0(void);
u64 longmode_get_cr4(void);
u64 longmode_get_efer(void);

/* --- 31E: kernel64 --- */
void kernel64_main(void *mbi);

/* --- 31F: idt64 + syscall --- */
void idt64_init(void);
void syscall64_init(void); /* MSR STAR/LSTAR/SFMASK */

/* --- 31G: gdt64 + tss --- */
void gdt64_load(void);
void tss64_init(void);

/* --- 31H: apic64 --- */
int apic64_init(void);
u64 apic64_base(void);

/* --- 31I: stack protector + relocator --- */
void stack_protector64_init(void);
int relocator64_check(u64 load_base, u64 link_base);

/* --- 32A: pmm64 (2GB cap bitmap, 4K frame) --- */
#define PMM64_MAX_MEMORY (2ULL*1024*1024*1024)
#define PMM64_FRAME 4096
void pmm64_init(void);
void pmm64_add_region(u64 base, u64 len);
void pmm64_reserve(u64 base, u64 len);
u64 pmm64_alloc_frame(void);
void pmm64_free_frame(u64 frame);
u64 pmm64_free_frames(void);
u64 pmm64_total_frames(void);
/* frame numarasi -> erisilebilir pointer.
 * Sozlesme: varsayilan identity (pointer == phys) YALNIZCA erken boot'ta
 * (identity-mapped) gecerlidir. Paging yeniden kurulunca kernel64 KENDI
 * mapper'ini pmm64_set_frame_mapper() ile kurmak ZORUNDADIR; aksi halde
 * slab/demand/cow/swap yanlis bellege yazar. NULL mapper -> identity'ye doner. */
void *pmm64_frame_ptr(u64 frame);
void pmm64_set_frame_mapper(void *(*fn)(u64 frame));

/* --- 32B: slab64 (8B..2K siniflar, pmm64 ustu) --- */
void *slab64_alloc(u64 size);
void slab64_free(void *p, u64 size);

/* --- 32C: demand paging --- */
#define D64_READ  (1ULL << 0)
#define D64_WRITE (1ULL << 1)
#define D64_EXEC  (1ULL << 2)
int demand64_add(u64 base, u64 len, u64 flags);
void demand64_remove(u64 base, u64 len);
/* fault_addr icin sifir sayfa tahsis eder; 0=cozuldu, <0 hata */
int demand64_fault(u64 fault_addr, u64 err_code, u64 *out_frame);

/* --- 32D: CoW fork cercevesi --- */
void cow64_retain(u64 frame);
int cow64_release(u64 frame); /* kalan referans */
int cow64_is_shared(u64 frame);
int cow64_resolve(u64 frame, u64 *out_new); /* kopyala-yaz: 0=ok */

/* --- 32E: KASLR --- */
u64 kaslr64_entropy(void);
u64 kaslr64_slide(u64 entropy, u64 align);
int kaslr64_verify(u64 load_base, u64 link_base, u64 slide);

/* --- 32F: per-CPU sayfa tablosu --- */
#define MAX_CPUS64 8
int percpu_pt_init(int ncpu);
int percpu_pt_clone(int cpu, u64 *src_pml4);
u64 *percpu_pt_get(int cpu);
void percpu_pt_switch(int cpu);

/* --- 32G: huge pages --- */
int paging64_build_identity(u64 *pml4, u64 *pdp, u64 *pd_pool,
                            int pd_count, u64 len, u64 flags);
void paging64_map_1gb_at(u64 *pdp, int idx, u64 phys, u64 flags);

/* --- 32H/32I: swap --- */
struct swap64_ops {
    int (*read_slot)(u64 slot, void *buf);
    int (*write_slot)(u64 slot, const void *buf);
    u64 slots;
};
int swap64_register(const struct swap64_ops *ops);
int swap64_alloc_slot(u64 *slot_out);
void swap64_free_slot(u64 slot);
int swap64_in(u64 slot, void *frame_buf);
int swap64_out(u64 slot, const void *frame_buf);
int swap64_swapon_ramdisk(u64 slots); /* 32I: RAM-disk backend */

/* --- 32J: bellek basincl --- */
int pressure64_level(void); /* 0..100 */
int pressure64_should_reclaim(void); /* 1 = <%5 bos */

/* --- 33A: SMP baslatma (INIT-SIPI-SIPI) ---
 * lapic: LAPIC MMIO tabani (gercek HW) veya test tamponu. */
#define LAPIC_ICR_LO 0x300
#define LAPIC_ICR_HI 0x310
#define LAPIC_SPURIOUS 0xF0
void smp64_delay(u64 loops);
void smp64_send_init(volatile u32 *lapic, u32 apic_id);
void smp64_send_sipi(volatile u32 *lapic, u32 apic_id, u8 vector);
int smp64_start_ap(volatile u32 *lapic, u32 apic_id, u8 vector);
/* --- 33B: AP trampoline --- */
u64 aptramp64_size(void); /* blob boyutu (<1KB) */
int aptramp64_check(const void *blob, u64 size); /* 0=saglam */
/* --- 33C: LAPIC timer kalibrasyon --- */
#define LAPIC_TMR_INIT 0x380
#define LAPIC_TMR_CUR 0x390
#define LAPIC_TMR_DIV 0x3E0
#define LAPIC_LVT_TMR 0x320
u32 lapic_timer64_calibrate(volatile u32 *lapic, u64 tsc_hz, u64 ms);
void lapic_timer64_start(volatile u32 *lapic, u8 vector, u32 count);
/* --- 33D: TLB shootdown IPI --- */
#define TLB64_VECTOR 0x50
void tlb64_init(void);
int tlb64_request(int cpu); /* 0=kuyruga alindi */
int tlb64_pending(int cpu);
void tlb64_ack(int cpu);
int tlb64_send(volatile u32 *lapic, u32 apic_id);
/* --- 33E: per-CPU workqueue --- */
#define WQ64_MAX 32
typedef void (*wq64_fn)(void *arg);
int wq64_init(void);
int wq64_enqueue(int cpu, wq64_fn fn, void *arg);
int wq64_run(int cpu); /* kuyrugu bosaltir, calisan adet */
int wq64_pending(int cpu);
/* --- 33F: scheduler affinity --- */
int aff64_init(int ncpu);
int aff64_set(int task, u64 mask);
u64 aff64_get(int task);
int aff64_pick(int task); /* maskeden en az yuklu CPU */
/* --- 33G: NUMA topology --- */
#define NUMA64_MAX_NODES 4
struct numa64_entry { u64 base; u64 len; u32 node; u32 reserved; };
int numa64_init(void);
int numa64_add(u64 base, u64 len, u32 node);
u32 numa64_node_of(u64 addr);
int numa64_nodes(void);
/* --- 33H: RCU skeleton --- */
void rcu64_init(void);
void rcu64_read_lock(void);
void rcu64_read_unlock(void);
void rcu64_call(void (*fn)(void *), void *arg);
void rcu64_quiescent(int cpu);
int rcu64_process(void); /*Hazir callback sayisi, calistirir */
/* --- 33I: lock-free kalloc (Treiber stack, CAS) --- */
void lf64_init(void *pool, u64 objsz, u64 count);
void *lf64_alloc(void);
void lf64_free(void *obj);

/* --- 34C: ACPI 64-bit (RSDP/XSDT saf ayrıştırma) --- */
int acpi64_checksum(const void *p, u64 len); /* 0=gecerli */
int acpi64_rsdp_check(const void *rsdp, u64 len); /* imza+checksum */
int acpi64_xsdt_entry(const void *xsdt, u64 len, int idx, u64 *out);
int acpi64_find(const void *xsdt, u64 len, const char sig[4], u64 *out);
/* --- 34D: GOP 64-bit framebuffer --- */
struct gop64_mode { u32 w, h, pitch_px, format; };
int gop64_validate(u32 w, u32 h, u32 pitch_px, u32 format); /* 0=ok */
u64 gop64_size_bytes(u32 h, u32 pitch_px);
u64 gop64_pages(u32 h, u32 pitch_px);
u32 gop64_fill_color(u32 rgb, u32 format); /* BGR ise R/B takas */
/* --- 34E: NVRAM boot girdisi --- */
#define NVRAM64_NV 0x00000001UL
#define NVRAM64_BS 0x00000002UL
#define NVRAM64_RT 0x00000004UL
int nvram64_boot_name(u32 idx, char out[9]); /* "Boot%04X" */
int nvram64_attrs_valid(u32 attrs);
/* --- 34F: Secure Boot shim politikasi --- */
int shim64_policy(int secure_boot, int setup_mode, int audit_mode);
/* 1=serbest, 2=denetimli serbest, 0=engelli (imza gerekli) */
/* --- 34G: runtime services --- */
int runtime64_keep(u32 efi_type); /* 1=SetVirtualAddressMap sonrasi korunur */
/* --- 34I: firmware degiskeni --- */
int var64_name_ok(const unsigned short *name); /* CHAR16: 1..64 ASCII */
int var64_size_ok(u64 size, u64 max);
int var64_guid_eq(const unsigned char a[16], const unsigned char b[16]);

/* --- 32A eki: menzil tahsisi (35A zone icin) --- */
u64 pmm64_alloc_range(u64 lo_frame, u64 hi_frame); /* [lo,hi) frame no */
/* --- 35A: zone yonetimi --- */
#define ZONE64_LOW 0   /* 0..1MB */
#define ZONE64_DMA 1   /* 1..16MB */
#define ZONE64_NORMAL 2 /* 16MB..2GB */
#define ZONE64_COUNT 3
int zone64_init(void);
u64 zone64_alloc(int zone);
void zone64_free(u64 frame);
u64 zone64_free_count(int zone);
u64 zone64_total_count(int zone);
/* --- 35B: buddy allocator (4K..2MB, order 0..9) --- */
int buddy64_init(u64 base_frame, int max_order);
u64 buddy64_alloc(int order);
void buddy64_free(u64 frame, int order);
u64 buddy64_free_count(int order);
/* --- 35C: swap LRU reclaim --- */
int reclaim64_init(void);
int reclaim64_add(u64 frame);
void reclaim64_touch(u64 frame);
int reclaim64_evict(int n, u64 *out); /* swap'a yazilmis frame sayisi */
/* --- 35D: OOM killer --- */
#define OOM64_MAX_TASKS 64
int oom64_add_task(int pid, u64 rss_pages, int adj);
int oom64_pick(void); /* en kotu pid, yoksa -1 */
int oom64_kill(int pid); /* 0=olduruldu */
int oom64_killed(int pid);
/* --- 35E: basincl bildirimi --- */
#define NOTIFY64_MAX 8
int notify64_register(void (*fn)(int level), int threshold);
int notify64_poll(void); /* ateslenen sayisi */
/* --- 35F: balloon --- */
int balloon64_inflate(u64 pages);
int balloon64_deflate(u64 pages);
u64 balloon64_pages(void);
/* --- 35G: KSM skeleton --- */
int ksm64_init(void);
int ksm64_scan(u64 frame); /* 1=birlesti, 0=yeni, <0 hata */
u64 ksm64_shared(void);
u64 ksm64_saved(void);
/* --- 35H: huge page swap (2MB = 512 slot) --- */
int hugeswap64_out(u64 huge_frame, u64 *slots_out);
int hugeswap64_in(u64 huge_frame, const u64 *slots);
/* --- 35I: memtest 64-bit --- */
int memtest64_run(void *buf, u64 len); /* 0=saglam, >0 ilk hata ofseti+1 */

/* --- 32B eki: sinif sorgu + shrink (36B/36I icin) --- */
int slab64_class_of(u64 size); /* -1 = karsiliksiz */
u64 slab64_class_size(int cls);
int slab64_shrink(void); /* tam bos slab'lari iade, adet */
/* --- 36A: cache coloring --- */
u64 cachecolor64_next(u64 objsz);
/* --- 36C: malloc profiling --- */
void *profile64_alloc(u64 size);
void profile64_free(void *p, u64 size);
int profile64_get(int cls, u64 *allocs, u64 *bytes, u64 *peak);
/* --- 36D: leak detector --- */
int leak64_add(void *ptr, u64 size);
void leak64_remove(void *ptr);
int leak64_count(void);
int leak64_get(int i, void **ptr, u64 *size);
/* --- 36E: KASAN-lite --- */
void *kasan64_alloc(u64 size);
int kasan64_check(const void *ptr, u64 size); /* 0=temiz */
void kasan64_free(void *ptr, u64 size);
/* --- 36F: guard pages --- */
u64 guard64_alloc_pages(int n);
int guard64_check(u64 base); /* 0=saglam */
void guard64_free(u64 base);
/* --- 36G: alignment --- */
u64 align64_up(u64 v, u64 a);
u64 align64_down(u64 v, u64 a);
int align64_is_pow2(u64 a);
void *align64_alloc(u64 size, u64 align);
void align64_free(void *p);
int align64_is_aligned(const void *p);
/* --- 36H: per-CPU arena --- */
void *arena64_alloc(int cpu, u64 size);
void arena64_free(int cpu, void *p, u64 size);
/* --- 37A: ASLR userspace --- */
u64 aslr64_stack_base(u64 entropy);
u64 aslr64_mmap_base(u64 entropy);
u64 aslr64_exec_base(u64 entropy, int pie);
/* --- 37B: NX zorunlulugu --- */
#define NX64_R (1ULL << 0)
#define NX64_W (1ULL << 1)
#define NX64_X (1ULL << 2)
u64 nx64_enforce(u64 req_flags, int is_stack_or_heap);
int nx64_exec_allowed(u64 flags);
u64 nx64_stack_flags(void);
/* --- 37C: kernel stack canary --- */
u64 scanary64_gen(u64 entropy);
int scanary64_verify(u64 stored, u64 current);
int scanary64_set_cpu(int cpu, u64 val);
u64 scanary64_get_cpu(int cpu);
/* --- 37D: seccomp filtre --- */
int seccomp64_add(int nr, int allow);
int seccomp64_check(int nr); /* 1=serbest, 0=engelli */
void seccomp64_strict(int on);
int seccomp64_count(void);
/* --- 37E: capabilities audit --- */
int capaudit64_grant(int cap);
int capaudit64_revoke(int cap);
int capaudit64_has(int cap);
int capaudit64_log(int pid, int cap, int granted);
int capaudit64_read(int i, int *pid, int *cap, int *granted);
/* --- 37F: SELinux skeleton --- */
struct selinux64_ctx { char user[32]; char role[32]; char type[32]; char level[32]; };
int selinux64_parse(const char *ctx, struct selinux64_ctx *out);
int selinux64_add_rule(const char *src, const char *dst, const char *cls, u32 perms);
int selinux64_check(const char *src, const char *dst, const char *cls, u32 perm);
void selinux64_enforcing(int on);
int selinux64_is_enforcing(void);
/* --- 37G: AppArmor skeleton --- */
int apparmor64_add_profile(const char *prefix, int enforce);
int apparmor64_add_rule(const char *prefix, const char *path, int op);
int apparmor64_check(const char *path, int op); /* op: 0=r,1=w,2=x */
#define APPARMOR64_R 0
#define APPARMOR64_W 1
#define APPARMOR64_X 2
/* --- 37H: user namespace --- */
int userns64_add_map(u32 ext_start, u32 int_start, u32 count);
int userns64_map_uid(u32 ext_uid, u32 *out);
int userns64_level(void);
void userns64_enter(void);
/* --- 37I: module signing --- */
u64 modsign64_hash(const void *data, u64 len);
int modsign64_add_key(int id, u64 hash);
int modsign64_verify(const void *data, u64 len, int key_id);
int modsign64_tainted(void);
/* --- 38A: passwd/shadow --- */
u64 passwd64_hash(const char *pw, const char *salt);
int passwd64_add(const char *user, u32 uid, u32 gid, const char *home,
                 const char *shell);
int passwd64_find(const char *user, u32 *uid, u32 *gid);
int shadow64_set(const char *user, u64 hash, u64 lastchg, u64 maxdays);
int shadow64_check(const char *user, u64 hash, u64 today);
/* --- 38B/38H: PAM iskeleti + modul API --- */
#define PAM64_REQUIRED 0
#define PAM64_REQUISITE 1
#define PAM64_SUFFICIENT 2
#define PAM64_OPTIONAL 3
typedef int (*pam64_fn)(int handle, const char *user);
int pam64_start(const char *service, const char *user);
int pam64_add(int handle, pam64_fn fn, int control);
int pam64_authenticate(int handle);
int pam64_acct_mgmt(int handle);
void pam64_end(int handle);
int pammod64_register(const char *name, pam64_fn auth, pam64_fn acct);
pam64_fn pammod64_find(const char *name, int phase);
/* --- 38C: sudo --- */
int sudo64_rule_add(const char *user, const char *target,
                    const char *command, int nopasswd);
int sudo64_check(const char *user, const char *target, const char *command,
                 int *need_pass);
int sudo64_stamp(const char *user, u64 now);
int sudo64_stamp_valid(const char *user, u64 now, u64 timeout);
/* --- 38D: capabilities v3 --- */
#define CAPV364_PERMITTED 0
#define CAPV364_EFFECTIVE 1
#define CAPV364_INHERITABLE 2
#define CAPV364_NSET 3
int capv364_set(int task, int which, u64 mask);
u64 capv364_get(int task, int which);
int capv364_has(int task, int which, int cap);
int capv364_exec(int task, u64 file_perm, u64 file_inh);
int capv364_bound_add(u64 mask);
u64 capv364_bound_get(void);
/* --- 38E: audit log --- */
int auditlog64_write(int type, int pid, const char *msg);
int auditlog64_read(u64 seq, int *type, int *pid, char *msg, int max);
u64 auditlog64_count_type(int type);
u64 auditlog64_next_seq(void);
/* --- 38F: group policy --- */
int group64_add(const char *name, u32 gid);
int group64_add_member(const char *name, const char *user);
int group64_is_member(const char *name, const char *user);
int group64_policy_add(const char *group, const char *resource, int perm);
int group64_check(const char *user, const char *resource, int perm);
/* --- 38G: SELinux context --- */
int selinuxctx64_add(const char *prefix, const char *ctx);
int selinuxctx64_label(const char *path, char *out, int max);
int selinuxctx64_add_trans(const char *src, const char *obj,
                           const char *cls, const char *dst);
int selinuxctx64_trans(const char *src, const char *obj, const char *cls,
                       char *out, int max);
/* --- 38I: privilege drop --- */
int privdrop64_set(u32 uid, u32 gid, u32 euid, u32 egid);
int privdrop64_drop(u32 uid, u32 gid);
int privdrop64_is_root(void);
int privdrop64_dumpable(void);
/* --- 39A: VFS inode ops --- */
#define VFS64_FILE 1
#define VFS64_DIR 2
#define VFS64_SYMLINK 3
int vfsops64_create(int type, u32 mode, u64 *ino_out);
int vfsops64_lookup(u64 ino, int *type, u32 *mode, u64 *size);
int vfsops64_setsize(u64 ino, u64 size);
int vfsops64_link(u64 ino);   /* nlink++ (39D hardlink) */
int vfsops64_nlink(u64 ino);
int vfsops64_unlink(u64 ino); /* nlink--, 0'da serbest */
/* --- 39B: dentry cache --- */
int dentry64_insert(u64 parent, const char *name, u64 ino);
int dentry64_lookup(u64 parent, const char *name, u64 *ino_out);
int dentry64_invalidate(u64 parent, const char *name);
int dentry64_count(void);
/* --- 39C: mount namespace --- */
int mountns64_mount(const char *path, const char *fs, u64 root_ino);
int mountns64_unmount(const char *path);
int mountns64_resolve(const char *path, char *inner, int max);
/* --- 39D: symlink/hardlink --- */
int symhard64_symlink(const char *target, u64 *ino_out);
int symhard64_readlink(u64 ino, char *out, int max);
int symhard64_hardlink(u64 ino); /* nlink++, kalan */
/* --- 39E: fcntl lock --- */
#define FLOCK64_SH 1
#define FLOCK64_EX 2
int flock64_lock(u64 ino, int owner, int type, u64 start, u64 len);
int flock64_unlock(u64 ino, int owner);
int flock64_check(u64 ino, int owner, int type, u64 start, u64 len);
/* --- 39F: inotify skeleton --- */
int inotify64_add_watch(u64 ino, u32 mask);
int inotify64_event(u64 ino, u32 mask);
int inotify64_read(int wd, u64 *ino, u32 *mask);
/* --- 39G: poll/epoll --- */
int poll64_set(int fd, u32 revents);
int poll64_poll(const int *fds, int nfds, u32 events, u32 *revents_out);
int epoll64_create(void);
int epoll64_add(int ep, int fd, u32 events);
int epoll64_mod(int ep, int fd, u32 events);
int epoll64_del(int ep, int fd);
int epoll64_wait(int ep, int *fds_out, u32 *ev_out, int max);
/* --- 39H: ioctl routing --- */
typedef int (*ioctl64_fn)(u64 arg);
int ioctl64_register(u64 dev, u32 cmd, ioctl64_fn fn);
int ioctl64_call(u64 dev, u32 cmd, u64 arg);
/* --- 39I: tmpfs --- */
int tmpfs64_create(const char *name);
int tmpfs64_write(const char *name, u64 off, const void *buf, u64 len);
int tmpfs64_read(const char *name, u64 off, void *buf, u64 len);
u64 tmpfs64_size(const char *name);
int tmpfs64_unlink(const char *name);
/* --- 40A: ext2 (1K blok, direkt bloklar) --- */
struct ext2_inode { u16 mode; u16 uid; u32 size; u32 gid; u16 links;
                    u32 blocks[15]; };
int ext2_attach(int (*rd)(u32 blk, void *buf), int (*wr)(u32 blk,
                                                         const void *buf));
int ext2_mount(void);
int ext2_super(u32 *blocks, u32 *inodes, u32 *bsize);
int ext2_read_inode(u32 ino, struct ext2_inode *out);
int ext2_read_file(u32 ino, u64 off, void *buf, u64 len);
int ext2_lookup(u32 dir_ino, const char *name, u32 *ino_out);
/* --- 40B: journal (write-ahead) --- */
int journal64_begin(void);
int journal64_write(u32 block, const void *data);
int journal64_commit(void); /* seq numarasi */
int journal64_abort(void);
int journal64_recover(void);
u64 journal64_seq(void);
void journal64_set_backend(int (*wr)(u32 blk, const void *buf));
/* --- 40C: fsync garantisi --- */
int fsync64_mark_dirty(u32 block, const void *data);
int fsync64_sync(void); /* yazilan sayisi */
int fsync64_barrier(void); /* bariyer seq */
u64 fsync64_barriers(void);
void fsync64_set_backend(int (*wr)(u32 blk, const void *buf));
/* --- 40D: xattr --- */
int xattr64_set(u64 ino, const char *name, const void *val, u64 len);
int xattr64_get(u64 ino, const char *name, void *out, u64 *len);
int xattr64_list(u64 ino, char *out, int max); /* NUL ayracli */
int xattr64_remove(u64 ino, const char *name);
/* --- 40E: POSIX ACL --- */
#define ACL64_USER_OBJ 1
#define ACL64_USER 2
#define ACL64_GROUP_OBJ 3
#define ACL64_GROUP 4
#define ACL64_MASK 5
#define ACL64_OTHER 6
int acl64_set(u64 ino, int tag, u32 id, int perm);
int acl64_check(u64 ino, u32 uid, u32 gid, int req);
/* --- 40F: kota --- */
int quota64_set_limit(u32 uid, u64 blim, u64 ilim);
int quota64_charge(u32 uid, u64 blocks, u64 inodes);
int quota64_release(u32 uid, u64 blocks, u64 inodes);
int quota64_check(u32 uid); /* 0=limit ici */
/* --- 40G: fsck --- */
int fsck64_run(void); /* bit bayrak: 0=saglam */
u64 fsck64_files(void);
u64 fsck64_blocks(void);
/* --- 40H: CoW snapshot --- */
int snap64_create(void); /* id, <0 hata */
int snap64_write(int id, u32 block, const void *old_data);
int snap64_read(int id, u32 block, void *out);
int snap64_delete(int id);
void snap64_set_backend(int (*rd)(u32 blk, void *buf));
/* --- 40I: FUSE arayuzu --- */
struct fuse64_ops {
    int (*lookup)(const char *path, u64 *ino_out);
    int (*read)(u64 ino, u64 off, void *buf, u64 len);
    int (*write)(u64 ino, u64 off, const void *buf, u64 len);
    int (*readdir)(u64 ino, void (*cb)(const char *name, u64 ino));
    int (*getattr)(u64 ino, int *type, u64 *size);
};
int fuse64_mount(const struct fuse64_ops *ops);
int fuse64_lookup(int h, const char *path, u64 *ino_out);
int fuse64_read(int h, u64 ino, u64 off, void *buf, u64 len);
int fuse64_write(int h, u64 ino, u64 off, const void *buf, u64 len);
int fuse64_getattr(int h, u64 ino, int *type, u64 *size);
/* --- 41A: .char paket formati --- */
#define CHARPKG64_MAGIC 0x52414843UL /* "CHAR" */
struct charpkg64_file { char path[64]; u64 offset; u64 size; u64 hash; };
struct charpkg64_hdr { u32 magic; char name[32]; char ver[16]; char arch[8];
                       u32 nfiles; };
int charpkg64_parse(const void *buf, u64 len, struct charpkg64_hdr *hdr,
                    struct charpkg64_file *files, int max, int *count_out);
int charpkg64_find(const struct charpkg64_file *files, int n,
                   const char *path, u64 *off, u64 *size);
/* --- 41B: repo index --- */
int repo64_add(const char *name, const char *ver, const char *repo);
int repo64_find(const char *name, const char *ver_req, char *ver_out,
                int max);
int repo64_vercmp(const char *a, const char *b);
int repo64_count(void);
/* --- 41C: dependency resolver --- */
struct depsolve64_dep { char name[32]; int op; char ver[16]; };
int depsolve64_parse_dep(const char *s, struct depsolve64_dep *out);
int depsolve64_solve(const char *name, char out[][32], int max);
int depsolve64_add_meta(const char *name, const char *ver,
                        const char deps[][48], int ndeps);
/* --- 41D: imza --- */
int sign64_add_key(int id, u64 secret);
u64 sign64_sign(const void *data, u64 len, int key_id);
int sign64_verify(const void *data, u64 len, u64 sig, int key_id);
/* --- 41E: transaction --- */
int txn64_begin(void);
int txn64_install(const char *name, const char *ver);
int txn64_remove(const char *name);
int txn64_commit(void);
int txn64_rollback(void);
int txn64_installed(const char *name, char *ver_out, int max);
/* --- 41F: atomic upgrade (A/B slot) --- */
int upgrade64_stage(const char *ver);
int upgrade64_commit(void);
int upgrade64_rollback(void);
int upgrade64_active(void); /* 0=A,1=B */
int upgrade64_version(int slot, char *out, int max);
/* --- 41G: mirror --- */
int mirror64_add(const char *url, int prio);
int mirror64_fail(const char *url);
int mirror64_sync_ok(const char *url, u64 now);
int mirror64_pick(char *out, int max);
/* --- 41H: flatpak runtime --- */
struct flatpak64_ref { char kind[16]; char name[64]; char arch[16];
                       char branch[16]; };
int flatpak64_parse_ref(const char *s, struct flatpak64_ref *out);
int flatpak64_add_runtime(const char *name, const char *arch,
                          const char *branch, const char *ver);
int flatpak64_find_runtime(const char *name, const char *arch,
                           const char *branch, char *ver_out, int max);
/* --- 41I: manifest import --- */
struct manifest64 { char name[64]; char runtime[64]; char sdk[64];
                    char branch[16]; char command[64]; };
int manifest64_parse(const char *buf, struct manifest64 *out);
int manifest64_to_pkg(const struct manifest64 *m);
/* --- 42A: init cekirdegi --- */
#define INIT64_STOPPED 0
#define INIT64_STARTING 1
#define INIT64_RUNNING 2
#define INIT64_STOPPING 3
#define INIT64_FAILED 4
int init64_add_service(const char *name);
int init64_start(const char *name);
int init64_stop(const char *name);
int init64_state(const char *name);
int init64_count_state(int state);
/* --- 42B: unit parser --- */
struct unit64 {
    char name[32];
    char desc[64];
    char exec[96];
    char after[8][32];
    int nafter;
    char wants[8][32];
    int nwants;
};
int unit64_parse(const char *buf, struct unit64 *out);
/* --- 42C: socket activation --- */
int sockact64_add(const char *sock, const char *service);
int sockact64_connection(const char *sock, char *service_out, int max);
int sockact64_pending(void);
/* --- 42D: journald --- */
int journald64_write(int prio, const char *service, const char *msg);
int journald64_query(const char *service, int idx, int *prio, char *msg,
                     int max);
int journald64_count(const char *service);
int journald64_vacuum(int keep);
/* --- 42E: loginctl --- */
int loginctl64_new_session(const char *user, const char *seat, int leader);
int loginctl64_activate(int id);
int loginctl64_terminate(int id);
int loginctl64_list(int *ids, int max);
int loginctl64_active(int *id_out);
/* --- 42F: cgroups v2 --- */
int cgroup64_mkdir(const char *path);
int cgroup64_set_limit(const char *path, const char *ctrl, u64 value);
int cgroup64_get_limit(const char *path, const char *ctrl, u64 *out);
int cgroup64_add_task(const char *path, int pid);
int cgroup64_tasks(const char *path);
/* --- 42G: timer units --- */
int timer64_add(const char *name, const char *service, u64 interval);
int timer64_tick(u64 now, char fired[][32], int max);
u64 timer64_next_in(u64 now);
/* --- 42H: boot target --- */
int boottarget64_add(const char *target, const char *wants);
int boottarget64_set_default(const char *target);
int boottarget64_plan(const char *target, char out[][32], int max);
/* --- 42I: rescue mode --- */
int rescue64_enter(const char *reason);
int rescue64_shell_ok(void);
int rescue64_exit(void);
int rescue64_active(void);
/* --- 43A: IP yigini --- */
int ipstack64_parse4(const char *s, u32 *out);
int ipstack64_fmt4(u32 ip, char *out, int max);
int ipstack64_parse6(const char *s, unsigned char out[16]);
int ipstack64_fmt6(const unsigned char ip[16], char *out, int max);
int ipstack64_is_v4mapped(const unsigned char ip[16]);
int ipstack64_subnet4(u32 ip, u32 net, u32 mask);
/* --- 43B: ARP/NDP --- */
int arpndp64_add4(u32 ip, const unsigned char mac[6], u64 now);
int arpndp64_lookup4(u32 ip, unsigned char mac[6], u64 now);
int arpndp64_add6(const unsigned char ip[16], const unsigned char mac[6],
                  u64 now);
int arpndp64_lookup6(const unsigned char ip[16], unsigned char mac[6],
                     u64 now);
int arpndp64_tick(u64 now);
/* --- 43C: TCP CUBIC --- */
struct cubic64 { u64 cwnd; u64 ssthresh; u64 wmax; u64 epoch; u64 origin; };
void cubic64_init(struct cubic64 *c);
void cubic64_ack(struct cubic64 *c, u64 rtt_us, u64 now_us);
void cubic64_loss(struct cubic64 *c);
/* --- 43D: UDP soket --- */
int udp64_socket(void);
int udp64_bind(int id, u32 ip, u16 port);
int udp64_connect(int id, u32 ip, u16 port);
int udp64_send(int id, const void *buf, u64 len);
int udp64_recv(int id, void *buf, u64 max);
int udp64_close(int id);
/* --- 43E: DHCP istemci --- */
int dhcp64_discover(u32 xid, unsigned char *out, int max);
int dhcp64_offer_parse(const unsigned char *msg, int len, u32 *ip,
                       u32 *mask, u32 *gw, u32 *dns, u32 *lease);
int dhcp64_request(u32 xid, u32 ip, unsigned char *out, int max);
int dhcp64_bound(u32 ip, u32 lease);
int dhcp64_bound_ip(u32 *ip, u32 *lease_left);
/* --- 43F: DNS cozucu --- */
int dns64_query(const char *name, unsigned char *out, int max);
int dns64_parse(const unsigned char *resp, int len, u32 *ips, int max);
int dns64_cached(const char *name, u32 *ip_out);
int dns64_cache_add(const char *name, u32 ip);
/* --- 43G: TLS 1.3 iskeleti --- */
#define TLS64_HELLO 1
#define TLS64_ESTABLISHED 5
int tls64_client_hello(unsigned char *out, int max);
int tls64_process(const unsigned char *msg, int len);
int tls64_state(void);
u64 tls64_transcript(void);
/* --- 43H: SLAAC --- */
int slaac64_eui64(const unsigned char mac[6], const unsigned char prefix[8],
                  unsigned char out[16]);
int slaac64_ra_parse(const unsigned char *ra, int len,
                     unsigned char prefix[8], u32 *lifetime);
int slaac64_dad_start(const unsigned char ip[16]);
int slaac64_dad_ok(void);
int slaac64_dad_conflict(void);
/* --- 43I: netfilter --- */
#define NETFILTER64_ACCEPT 1
#define NETFILTER64_DROP 0
#define NETFILTER64_LOG 2
struct netfilter64_pkt { u32 src, dst; u8 proto; u16 sport, dport; };
int netfilter64_add(int chain, u32 src, u32 smask, u32 dst, u32 dmask,
                    u8 proto, u16 dport, int verdict);
int netfilter64_hook(int chain, const struct netfilter64_pkt *pkt);
/* --- 44A: RTL8821CE probe --- */
#define RTL8821CE_VENDOR 0x10EC
#define RTL8821CE_DEVICE 0xC821
int rtlprobe64_match(u16 vendor, u16 device);
int rtlprobe64_rev(u32 reg_id); /* <0 bilinmiyor */
int rtlprobe64_check_bar(u64 bar, u64 size);
/* --- 44B: firmware yukleme --- */
struct fwload64_chunk { u32 addr; u32 len; const unsigned char *data; };
int fwload64_parse(const void *blob, u64 len, u32 *ver_out);
int fwload64_nchunks(void);
int fwload64_chunk(int i, struct fwload64_chunk *out);
int fwload64_verify(void);
int fwload64_next(int *state); /* sonraki idx, -1=bitti */
/* --- 44C: mac80211 iskeleti --- */
#define MAC11_STA 0
#define MAC11_AP 1
#define MAC11_MONITOR 2
#define MAC11_AC_VO 0
#define MAC11_AC_VI 1
#define MAC11_AC_BE 2
#define MAC11_AC_BK 3
int mac11_add_iface(int type, const unsigned char mac[6]);
int mac11_tx(int ac, const void *frame, u64 len);
int mac11_rx_pending(int ac);
int mac11_rx(int ac, void *buf, u64 max);
int mac11_set_key(int idx, const unsigned char key[16]);
/* --- 44D: WPA3 el-sikisma --- */
int wpa364_sae_commit(const char *password, u64 *scalar_out, u64 *elem_out);
int wpa364_sae_confirm(u64 peer_scalar, u64 peer_elem, u64 *confirm_out);
int wpa364_sae_verify(u64 confirm, u64 peer_scalar, u64 peer_elem);
int wpa364_4way_msg123(const unsigned char anonce[32], u64 *ptk_out);
int wpa364_4way_msg4(u64 mic);
int wpa364_state(void);
/* --- 44E: tarama/dolasim --- */
int scan64_add(const unsigned char bssid[6], const char *ssid, int chan,
               int rssi, int rsn);
int scan64_best(const char *ssid, unsigned char bssid[6], int *rssi);
int scan64_roam_needed(int cur_rssi, int best_rssi);
int scan64_count(void);
/* --- 44F: guc tasarrufu --- */
#define POWERSAVE64_ACTIVE 0
#define POWERSAVE64_LIGHT 1
#define POWERSAVE64_DEEP 2
int powersave64_set(int mode);
int powersave64_mode(void);
int powersave64_beacon(int dtim_count, int buffered);
u64 powersave64_saved_us(void);
/* --- 44G: AP kipi --- */
int apmode64_sta_add(const unsigned char mac[6]);
int apmode64_sta_authorize(const unsigned char mac[6]);
int apmode64_sta_remove(const unsigned char mac[6]);
int apmode64_beacon(const char *ssid, int chan, unsigned char *out,
                    int max);
int apmode64_clients(void);
/* --- 44H: mesh iskeleti --- */
u64 mesh64_metric(u64 rate_mbps, u64 errors);
int mesh64_learn(const unsigned char dest[6], const unsigned char hop[6],
                 u64 metric);
int mesh64_route(const unsigned char dest[6], unsigned char hop[6]);
/* --- 44I: surucu oz-test --- */
int drvtest64_regs(u32 id_val);
int drvtest64_loopback(const void *frame, u64 len);
int drvtest64_fw_alive(u32 heartbeat);
/* --- 45A: UAS mass storage --- */
struct uas64_cmd { u8 iu_id; u8 rsvd; u16 tag; u8 prio; u8 lun;
                   u8 cdb_len; u8 cdb[16]; };
int uas64_build_cmd(u8 lun, const u8 *cdb, int cdb_len, u16 tag,
                    struct uas64_cmd *out);
int uas64_parse_sense(const unsigned char *iu, int len, u8 *key, u8 *asc);
int uas64_tag_alloc(void);
void uas64_tag_free(int tag);
int uas64_abort(u16 tag);
/* --- 45B: USB audio --- */
int usbaudio64_parse_rates(const unsigned char *desc, int len, u32 *out,
                           int max);
int usbaudio64_rate_set(u32 rate);
u32 usbaudio64_rate_get(void);
int usbaudio64_volume_set(int ch, int vol_db);
int usbaudio64_volume_get(int ch);
int usbaudio64_mute(int ch, int on);
/* --- 45C: USB video (UVC) --- */
int uvc64_probe(u32 w, u32 h, u32 fps, int fmt, u64 *bw_out);
int uvc64_bandwidth(u32 w, u32 h, u32 fps, int fmt, u64 *out);
int uvc64_format_guid(int fmt, unsigned char guid[16]);
/* --- 45D: hub power --- */
#define HUBPOWER64_OFF 0
#define HUBPOWER64_ON 1
#define HUBPOWER64_SUSPEND 2
int hubpower64_set(int port, int state);
int hubpower64_state(int port);
int hubpower64_overcurrent(int port);
int hubpower64_budget(int speed_mbps, int *ma_out);
/* --- 45E: xHCI MSI --- */
int msi64_parse(u16 control, int *is64, int *mmc);
int msi64_alloc_vector(int *vec_out);
void msi64_free_vector(int vec);
int msi64_msg(u64 *addr, u32 *data, int vec, int apic_id);
int msi64_msix_entry(const unsigned char *e, u64 *addr, u32 *data,
                     u32 *ctrl);
/* --- 45F: USB/IP --- */
#define USBIP64_SUBMIT 1
#define USBIP64_UNLINK 2
#define USBIP64_RET_SUBMIT 3
int usbip64_build_submit(u32 seq, u32 devid, u8 ep, u32 flags,
                         unsigned char *out, int max);
int usbip64_parse_ret(const unsigned char *msg, int len, u32 *seq,
                      int *status);
int usbip64_build_unlink(u32 seq, u32 unseq, unsigned char *out, int max);
/* --- 45G: HID report parser --- */
typedef void (*hidparse64_cb)(u16 usage_page, u16 usage, u32 bitpos,
                              u32 bits, int is_input);
int hidparse64_walk(const unsigned char *desc, int len, hidparse64_cb cb);
int hidparse64_keyboard(const unsigned char *desc, int len, u32 *keybits,
                        u32 *nkeys);
/* --- 45H: virtual HID --- */
int virthid64_create(const unsigned char *desc, int len);
int virthid64_inject_key(u8 mod, const u8 keys[6]);
int virthid64_inject_mouse(u8 buttons, int dx, int dy);
int virthid64_read(unsigned char *out, int max);
/* --- 45I: USB devfs --- */
int usbdevfs64_add(int bus, int addr, u16 vid, u16 pid, int speed);
int usbdevfs64_remove(int bus, int addr);
int usbdevfs64_find(u16 vid, u16 pid, int *bus, int *addr);
int usbdevfs64_desc(int bus, int addr, const unsigned char *desc, int len);
int usbdevfs64_read_desc(int bus, int addr, unsigned char *out, int max);
/* --- 46A: HD Audio codec --- */
u32 hdacodec64_verb(u8 nid, u16 verb, u16 param);
int hdacodec64_pin_config(u32 resp, int *loc, int *dev, int *conn,
                          int *color);
int hdacodec64_widget_type(u32 param);
int hdacodec64_fg_scan(const u32 *resps, int n, u8 *nids, int max);
/* --- 46B: ALSA PCM --- */
#define ALSA64_CLOSED 0
#define ALSA64_SETUP 1
#define ALSA64_PREPARED 2
#define ALSA64_RUNNING 3
#define ALSA64_XRUN 4
int alsa64_open(u32 rate, int ch, int fmt);
int alsa64_hw_params(u32 rate, int ch, u32 *buf_frames, u32 *period);
int alsa64_prepare(void);
int alsa64_writei(const short *buf, u32 frames);
u32 alsa64_avail(void);
int alsa64_state(void);
int alsa64_close(void);
/* --- 46C: mixer --- */
int mixer64_add(const char *name, int min, int max);
int mixer64_set(const char *name, int ch, int val);
int mixer64_get(const char *name, int ch);
int mixer64_db_to_reg(int db);
int mixer64_reg_to_db(int reg);
/* --- 46D: PulseAudio server --- */
int pulse64_client_add(const char *name);
int pulse64_stream_open(int client, u32 rate, int ch);
int pulse64_mix(short *out, int frames);
int pulse64_set_volume(int stream, int vol);
int pulse64_write(int stream, const short *buf, int frames);
/* --- 46E: JACK skeleton --- */
#define JACK64_AUDIO 0
#define JACK64_MIDI 1
int jack64_port_register(const char *name, int type, int is_input);
int jack64_connect(int src, int dst);
int jack64_cycle(void); /* islenen baglanti sayisi */
int jack64_set_rate(u32 rate, u32 bufsize);
/* --- 46F: Bluetooth audio --- */
int btaudio64_sbc_config(u32 rate, int blocks, int subbands, int bitpool,
                         u32 *bitrate_out);
int btaudio64_avrcp(int cmd, int *opcode_out);
int btaudio64_connect(const unsigned char mac[6]);
/* --- 46G: sample rate conv --- */
int src64_open(u32 in_rate, u32 out_rate);
int src64_process(const short *in, int n, short *out, int max);
u64 src64_ratio(void); /* x65536 */
/* --- 46H: low latency --- */
u32 lowlat64_period_frames(u32 rate, u32 latency_us);
u32 lowlat64_latency_us(u32 rate, u32 period_frames);
u32 lowlat64_watermark(u32 period_frames);
int lowlat64_policy(int realtime);
/* --- 46I: sound test --- */
int sndtest64_sine(u32 freq, u32 rate, int n, short *out);
int sndtest64_sweep(u32 f0, u32 f1, u32 rate, int n, short *out);
int sndtest64_silence(const short *buf, int n);
int sndtest64_clipping(const short *buf, int n);
/* --- 46J: media player --- */
#define MEDIA64_STOPPED 0
#define MEDIA64_PLAYING 1
#define MEDIA64_PAUSED 2
int media64_playlist_add(const char *path, u64 duration_ms);
int media64_play(void);
int media64_pause(void);
int media64_stop(void);
int media64_next(void);
int media64_prev(void);
int media64_seek(u64 ms);
u64 media64_position(u64 elapsed_ms);
int media64_state(void);
/* --- 47A: Gallium low profile --- */
int gallium64_init(u64 vram_mb);
int gallium64_caps(u32 *max_tex, int *low_profile);
int gallium64_format_ok(u32 format);
/* --- 47B: KMS --- */
int kms64_add_connector(const char *name);
int kms64_add_mode(int conn, u32 w, u32 h, u32 refresh);
int kms64_set_mode(int conn, u32 w, u32 h);
int kms64_current(int conn, u32 *w, u32 *h, u32 *refresh);
/* --- 47C: Vulkan 1.0 low --- */
int vk64_add_gpu(const char *name, u64 vram_mb);
int vk64_pick_gpu(u64 min_vram);
int vk64_version_check(u32 major, u32 minor);
int vk64_queue_count(int gpu);
/* --- 47D: OpenGL ES 3.2 --- */
int gles64_context(u32 major, u32 minor, u32 w, u32 h);
int gles64_viewport(u32 x, u32 y, u32 w, u32 h);
int gles64_shader(int type, const char *src);
int gles64_link(int vs, int fs);
/* --- 47E: VRAM 512MB optimizasyon --- */
int vram64_init(u64 size);
u64 vram64_alloc(u64 size, u64 align);
void vram64_free(u64 addr);
int vram64_fragmentation(void); /* 0..100 */
int vram64_mark_purgeable(u64 addr);
int vram64_purge(void); /* geri alinan bayt */
/* --- 47F: 3D compositor GL --- */
int glcomp64_surface(u32 w, u32 h, u32 color);
int glcomp64_damage(int id, u32 x, u32 y, u32 w, u32 h);
u32 glcomp64_present(void); /* ilk piksel karmasi */
u64 glcomp64_vsync(void);
/* --- 47G: GPU scheduler --- */
int sched64_ctx_add(int prio, u64 quantum_us);
int sched64_submit(int ctx, u64 jobs);
u64 sched64_run(void); /* tamamlanan is */
int sched64_preempt(int ctx);
/* --- 47H: compute shaders --- */
u64 compute64_groups(u64 total, u64 local);
int compute64_dispatch(u64 gx, u64 gy, u64 gz, u64 local);
int compute64_shared_ok(u64 bytes);
u64 compute64_completed(void);
/* --- 47I: VA-API decode --- */
int vaapi64_profile_ok(int profile);
int vaapi64_config(int profile, u32 w, u32 h);
int vaapi64_surfaces(int n, u32 w, u32 h);
int vaapi64_decode(int surf, const void *data, u64 len);
u64 vaapi64_frames(void);
/* --- 48A: ACPI parse --- */
struct acpiparse64_fadt { u32 dsdt; u32 smi_cmd; u32 pm1a_cnt; u32 pm1b_cnt;
                          u8 slp_typ_a; u8 slp_typ_b; };
int acpiparse64_fadt(const void *xsdt, u64 len, struct acpiparse64_fadt *out);
int acpiparse64_sleep(const struct acpiparse64_fadt *fadt, u8 *a, u8 *b);
/* --- 48B: S3 --- */
int s364_register(const char *dev, int (*suspend)(void),
                  int (*resume)(void));
int s364_suspend(u8 typ_a, u8 typ_b);
int s364_resume(void);
int s364_state(void); /* 0=acik,1=askida,2=uyaniyor */
/* --- 48C: S4 --- */
int s464_begin(u64 pages);
int s464_write_page(u64 idx, const void *data);
int s464_commit(void);
int s464_restore_verify(void);
/* --- 48D: cpufreq --- */
int cpufreq64_add_state(u32 mhz, u32 mw);
int cpufreq64_governor(int load_pct); /* secilen MHz */
int cpufreq64_set(u32 mhz);
u32 cpufreq64_get(void);
/* --- 48E: cpuidle --- */
int cpuidle64_add_state(const char *name, u32 latency_us, u32 power);
int cpuidle64_pick(u32 idle_us);
int cpuidle64_residency(int idx, u64 us);
/* --- 48F: power button --- */
int powerbtn64_event(u32 duration_ms);
int powerbtn64_read(void); /* 0=yok,1=kisa,2=uzun */
int powerbtn64_action(int ev); /* 0=hickimse,1=askiya,2=kapat */
/* --- 48G: battery --- */
int battery64_update(u64 design, u64 full, u64 cur, u32 mv, int charging);
void battery64_set_rate(int mw);
int battery64_pct(void);
int battery64_state(void); /* 0=bilinmiyor,1=sarj,2=desarj,3=dolu */
u64 battery64_minutes(void);
/* --- 48H: thermal trip --- */
int thermal64_set_temp(int zone, int temp_c);
int thermal64_check(int zone); /* 0=ok,1=pasif,2=kritik */
int thermal64_cooling(int zone, int level);
/* --- 48I: WoL --- */
int wol64_magic(const unsigned char mac[6], unsigned char *out);
int wol64_add_pattern(u32 off, const unsigned char *mask,
                      const unsigned char *pat, int len);
int wol64_match(const unsigned char *frame, int len);
/* --- 49A: hwmon --- */
#define HWMON64_TEMP 0
#define HWMON64_FAN 1
#define HWMON64_VOLT 2
#define HWMON64_POWER 3
int hwmon64_add(const char *name, int type);
int hwmon64_update(const char *name, int value);
int hwmon64_read(const char *name, int *value);
/* --- 49B: CPU temp --- */
int cputemp64_update(int core, u32 tjmax, u32 readout);
int cputemp64_core(int core);
int cputemp64_max(void);
/* --- 49C: fan control --- */
int fan64_set_target(int fan, int temp_c);
int fan64_tick(int fan, int temp_c);
int fan64_pwm(int fan);
int fan64_curve(int temp_c);
/* --- 49D: battery status --- */
int battstat64_snapshot(int pct, int charging, u64 rate_mw, u64 cap_mwh);
int battstat64_pct(void);
int battstat64_minutes(void);
/* --- 49E: system monitor --- */
void sysmon64_tick(u64 cpu, u64 idle, u64 mem);
int sysmon64_cpu_pct(void);
u64 sysmon64_mem(void);
void sysmon64_io(u64 rd, u64 wr, u64 rx, u64 tx);
/* --- 49F: prometheus --- */
int prom64_gauge(const char *name, const char *help, u64 value);
int prom64_inc(const char *name);
int prom64_render(char *out, int max);
/* --- 49G: SMART --- */
int smart64_add(int id, int value, int worst, int thresh, u64 raw);
int smart64_failing(void);
int smart64_health(void); /* 1=saglam, 0=bozuk */
int smart64_temp(void);
/* --- 49H: logrotate --- */
int logrotate64_add(const char *name, u64 maxsize, int keep);
int logrotate64_write(const char *name, u64 bytes);
int logrotate64_generations(const char *name);
/* --- 49I: health test --- */
int health64_add(const char *name, int (*fn)(void));
int health64_run(void); /* basarisiz sayisi */
u64 health64_status(void); /* bit maskesi */
/* --- 50A: btrfs skeleton --- */
int btrfs64_subvol_create(const char *name, u64 parent);
int btrfs64_subvol_delete(const char *name);
int btrfs64_snapshot(const char *src, const char *name);
int btrfs64_list(char out[][48], int max);
/* --- 50B: ZFS --- */
int zfs64_pool_create(const char *pool, u64 size);
int zfs64_dataset_create(const char *pool, const char *name,
                         const char *mount);
int zfs64_scrub(const char *pool);
int zfs64_health(const char *pool); /* 0=saglikli */
/* --- 50C: sshfs --- */
int sshfs64_connect(const char *host, const char *user, int port);
int sshfs64_auth(const char *password);
int sshfs64_packet(u32 type, u32 id, const void *payload, u64 len,
                   unsigned char *out, int max);
int sshfs64_attr_parse(const unsigned char *p, int len, u64 *size,
                       u32 *mode);
/* --- 50D: NFS client --- */
int nfsclient64_mount(const char *host, const char *path);
int nfsclient64_rpc(u32 prog, u32 vers, u32 proc, u32 xid,
                    unsigned char *out, int max);
int nfsclient64_read(u64 fh, u64 off, void *buf, u64 len);
int nfsclient64_write(u64 fh, u64 off, const void *buf, u64 len);
void nfsclient64_set_backend(int (*rd)(u64, u64, void *, u64),
                             int (*wr)(u64, u64, const void *, u64));
/* --- 50E: NFS server --- */
int nfsserver64_export(const char *path, const char *clients);
int nfsserver64_dispatch(u32 proc, u64 arg, u64 *result);
int nfsserver64_clients(const char *path);
/* --- 50F: SMB client --- */
int smb64_negotiate(const u32 *dialects, int n);
int smb64_tree(const char *share);
int smb64_read(u64 fh, u64 off, void *buf, u64 len);
int smb64_write(u64 fh, u64 off, const void *buf, u64 len);
void smb64_set_backend(int (*rd)(u64, u64, void *, u64),
                       int (*wr)(u64, u64, const void *, u64));
/* --- 50G: LUKS --- */
int luks64_format(const char *cipher);
int luks64_add_key(int slot, u64 hash);
int luks64_unlock(int slot, u64 hash);
int luks64_lock(void);
int luks64_is_open(void);
/* --- 50H: overlayfs --- */
int overlay64_mount(const char *upper, const char *workdir);
int overlay64_add_lower(const char *path);
int overlay64_whiteout(const char *path);
int overlay64_lookup(const char *path, char *layer_out, int max);
int overlay64_copy_up(const char *path);
/* --- 50I: encrypted FS (YER_TUTUCU akis sifresi; gercek kripto 51x) --- */
int encfs64_set_key(const unsigned char key[32]);
int encfs64_encrypt(u64 nonce, const void *in, void *out, u64 len);
int encfs64_decrypt(u64 nonce, const void *in, void *out, u64 len);
int encfs64_name(const char *name, char *out, int max);
/* --- 51A: musl cekirdek ilkeller --- */
void *musl64_memcpy(void *d, const void *s, u64 n);
void *musl64_memmove(void *d, const void *s, u64 n);
void *musl64_memset(void *d, int c, u64 n);
u64 musl64_strlen(const char *s);
int musl64_strcmp(const char *a, const char *b);
int musl64_version(char *out, int max);
/* --- 51B: pthread --- */
int pthread64_create(int *id_out);
int pthread64_join(int id, int *retval);
int pthread64_detach(int id);
int pthread64_self(void);
int pthread64_mutex_init(void);
int pthread64_mutex_lock(int m);
int pthread64_mutex_trylock(int m);
int pthread64_mutex_unlock(int m);
int pthread64_cond_wait(int c, int m);
int pthread64_cond_signal(int c);
int pthread64_cond_broadcast(int c);
/* --- 51C: DNS resolver --- */
int dnsresolv64_nameserver(const char *ip);
int dnsresolv64_search(const char *domain);
int dnsresolv64_add_host(const char *name, const char *ip);
int dnsresolv64_lookup(const char *name, char *out, int max);
/* --- 51D: stdio buffering --- */
#define STDIO64_NONE 0
#define STDIO64_LINE 1
#define STDIO64_FULL 2
int stdio64_open(int mode);
int stdio64_write(int f, const void *buf, u64 len);
int stdio64_read(int f, void *buf, u64 len);
int stdio64_flush(int f);
int stdio64_seek(int f, u64 off);
int stdio64_close(int f);
/* --- 51E: locale --- */
int locale64_set(const char *name);
int locale64_conv(char *dec, char *thousands, char *currency, int max);
int locale64_format_num(long long v, char *out, int max);
/* --- 51F: time zone --- */
int tz64_add_rule(const char *name, int std_min, int dst_min, int sm,
                  int sw, int em, int ew);
int tz64_offset(const char *name, int y, int mon, int day, int hour);
long long tz64_utc_to_local(const char *name, long long utc,
                            int y, int mon, int day, int hour);
/* --- 51G: iconv --- */
int iconv64_open(const char *to, const char *from);
int iconv64_convert(int cd, const unsigned char **in, u64 *inleft,
                    unsigned char **out, u64 *outleft);
int iconv64_close(int cd);
/* --- 51H: regex --- */
int regex64_compile(const char *pat);
int regex64_match(int re, const char *s);
int regex64_search(int re, const char *s, int *start_out);
/* --- 51I: math lib --- */
u64 math64_isqrt(u64 x);
u64 math64_icbrt(u64 x);
int math64_ilog2(u64 x);
u64 math64_pow_u64(u64 base, int exp);
u64 math64_gcd(u64 a, u64 b);
int math64_sin_q16(int degrees);
int math64_cos_q16(int degrees);
/* --- 52A: ELF64 parser --- */
struct elfparse64_hdr { u16 type, machine; u64 entry, phoff, shoff;
                        int phnum, shnum; };
int elfparse64_header(const void *buf, u64 len, struct elfparse64_hdr *out);
int elfparse64_section(const void *buf, u64 len, int idx, u64 *off,
                       u64 *size, u32 *type);
/* --- 52B: program headers --- */
int progheader64_each(const void *buf, u64 len,
                      int (*cb)(u32 type, u64 off, u64 vaddr, u64 filesz,
                                u64 memsz, u32 flags));
int progheader64_interp(const void *buf, u64 len, char *out, int max);
int progheader64_stack_exec(const void *buf, u64 len);
/* --- 52C: dynamic linker --- */
int dynlink64_get(const void *dyn, int nent, long tag, u64 *val_out);
int dynlink64_needed(const void *dyn, int nent, const char *strtab,
                     u64 strsz, char out[][32], int max);
int dynlink64_str(const void *dyn, int nent, long tag, const char *strtab,
                  u64 strsz, char *out, int max);
/* --- 52D: .so load --- */
struct soload64_seg { u64 vaddr, memsz, filesz, flags; };
int soload64_plan(const void *buf, u64 len, u64 bias,
                  struct soload64_seg *segs, int max, int *count_out,
                  u64 *total_out);
/* --- 52E: ASLR userspace --- */
u64 aslruser64_mmap_base(u64 entropy);
u64 aslruser64_stack(u64 entropy);
u64 aslruser64_brk(u64 entropy);
u64 aslruser64_pie_bias(u64 entropy);
/* --- 52F: RPATH/RUNPATH --- */
int rpath64_add_file(const char *dir, const char *lib);
int rpath64_has(const char *dir, const char *lib);
int rpath64_find(const char *rpath, const char *runpath,
                 const char *ldpath, const char *lib, char *out, int max);
/* --- 52G: delay load --- */
int delayload64_register(const char *sym);
int delayload64_resolve(const char *sym, u64 addr);
u64 delayload64_call(const char *sym);
/* --- 52H: symbol versioning --- */
int symver64_add_def(const char *name, const char *ver);
int symver64_need(const char *name, const char *ver);
int symver64_check(void); /* 0=tumu saglandi */
/* --- 52I: dlopen/dlsym --- */
int dlopen64_open(const char *path, int global);
int dlopen64_add_sym(int h, const char *name, u64 addr);
u64 dlopen64_sym(int h, const char *name);
int dlopen64_close(int h);
/* --- 53A: prelink --- */
int prelink64_assign(const char *libs[], const u64 sizes[], int n, u64 base,
                     u64 *addrs_out);
int prelink64_verify(const u64 *addrs, const u64 *sizes, int n);
/* --- 53B: LD_LIBRARY_PATH --- */
int ldpath64_parse(const char *list, char out[][96], int max);
int ldpath64_expand(const char *path, const char *origin, char *out,
                    int max);
int ldpath64_find(const char *list, const char *lib, char *out, int max);
/* --- 53C: NSS --- */
int nss64_add(const char *db, const char *module);
int nss64_module_add(const char *db, const char *module, const char *key,
                     const char *value);
int nss64_lookup(const char *db, const char *key, char *out, int max);
/* --- 53D: iconv provider --- */
typedef int (*iconvprov64_fn)(const unsigned char *in, u64 ilen,
                              unsigned char *out, u64 *olen);
int iconvprov64_register(const char *from, const char *to,
                         iconvprov64_fn fn);
int iconvprov64_convert(const char *from, const char *to,
                        const unsigned char *in, u64 ilen,
                        unsigned char *out, u64 *olen);
/* --- 53E: crypto provider --- */
typedef int (*cryptoprov64_hashfn)(const void *data, u64 len, u64 *out);
typedef int (*cryptoprov64_cryptfn)(const unsigned char *key,
                                    const unsigned char *in,
                                    unsigned char *out, u64 len);
int cryptoprov64_register_hash(const char *name, cryptoprov64_hashfn fn);
int cryptoprov64_register_cipher(const char *name,
                                 cryptoprov64_cryptfn fn);
int cryptoprov64_hash(const char *name, const void *data, u64 len,
                      u64 *out);
int cryptoprov64_crypt(const char *name, const unsigned char *key,
                       const unsigned char *in, unsigned char *out,
                       u64 len);
/* --- 53F: plugin API --- */
typedef int (*plugin64_entry)(int op, u64 arg);
int plugin64_load(const char *name, const char *ver, plugin64_entry entry);
int plugin64_unload(const char *name);
int plugin64_call(const char *name, int op, u64 arg);
int plugin64_add_dep(const char *plugin, const char *dep);
int plugin64_check_deps(const char *plugin);
/* --- 53G: module hotplug --- */
int hotplug64_insert(const char *name);
int hotplug64_remove(const char *name);
int hotplug64_event(const char *dev, int action);
int hotplug64_poll(char *dev_out, int max, int *action_out);
/* --- 53H: self-test --- */
int selftest64_add(const char *name, int (*fn)(void));
int selftest64_run(void); /* basarisiz sayisi */
int selftest64_result(int i, char *name_out, int max);
/* --- 53I: link test --- */
int linktest64_undef(const char **syms, int n);
int linktest64_reloc(u64 base);
/* --- 54A: shell --- */
int shell64_tokenize(const char *line, char out[][64], int max);
int shell64_expand(const char *in, const char *var, const char *val,
                   char *out, int max);
int shell64_builtin(const char *cmd, int argc, char argv[][64],
                    char *out, int max);
int shell64_history_add(const char *line);
int shell64_history_get(int idx, char *out, int max);
/* --- 54B: coreutils --- */
int coreutils64_exists(const char *name);
int coreutils64_list(int cat, char out[][32], int max);
int coreutils64_echo(int argc, char argv[][64], char *out, int max);
int coreutils64_wc(const char *text, u64 *lines, u64 *words, u64 *bytes);
/* --- 54C: busybox --- */
int busybox64_applet(const char *name, int argc, char argv[][64],
                     char *out, int max);
/* --- 54D: textutils --- */
int textutils64_grep(const char *pat, const char *text, char *out, int max);
int textutils64_find(const char *files[], int n, const char *suffix,
                     char out[][64], int max);
int textutils64_sed_replace(const char *text, const char *from,
                            const char *to, char *out, int max);
long long textutils64_awk_sum(const char *text, int col);
/* --- 54E: ssh client --- */
int sshclient64_packet(u8 type, const void *payload, u64 len,
                       unsigned char *out, int max);
int sshclient64_banner(const char *line, char *ver_out, int max);
int sshclient64_kexinit(unsigned char *out, int max);
/* --- 54F: git client --- */
int gitclient64_sha1(const void *data, u64 len, unsigned char out[20]);
int gitclient64_object(const char *type, const void *data, u64 len,
                       unsigned char *out, int max);
int gitclient64_ref(const char *line, char *hash_out, char *name_out);
/* --- 54G: make --- */
int make64_add_rule(const char *target, const char *deps[],
                    const char *recipe);
int make64_set_time(const char *target, u64 mtime);
int make64_build(const char *target, char built[][48], int max);
/* --- 54H: python runtime skeleton --- */
#define PYTHON64_INT 0
#define PYTHON64_STR 1
struct python64_obj { int type; long long ival; char sval[64]; };
int python64_int(long long v, struct python64_obj *out);
int python64_str(const char *s, struct python64_obj *out);
int python64_add(const struct python64_obj *a,
                 const struct python64_obj *b, struct python64_obj *out);
long long python64_eval_int(const char *expr, int *ok);
int python64_print(const struct python64_obj *o, char *out, int max);
/* --- 54I: node skeleton --- */
int node64_require(const char *name);
int node64_set_timeout(u64 ms, int cb, u64 now);
int node64_next_tick(int cb);
int node64_tick(u64 now, int *fired, int max);
/* --- 55A: wayland compositor --- */
typedef int (*wayland64_fn)(u32 obj, u16 opcode, u64 arg);
int wayland64_global_add(const char *interface, u32 version);
int wayland64_bind(const char *interface, u32 version, u32 *id_out);
int wayland64_method(u32 obj, const char *interface, u16 opcode,
                     wayland64_fn fn);
int wayland64_dispatch(u32 obj, u16 opcode, u64 arg);
int wayland64_event(u32 obj, u16 opcode, u64 arg);
int wayland64_poll(u32 *obj, u16 *opcode, u64 *arg);
/* --- 55B: X11 compat --- */
int xcompat64_window_create(u32 xid, int surf);
int xcompat64_prop_set(u32 xid, const char *name, const char *val);
int xcompat64_prop_get(u32 xid, const char *name, char *out, int max);
int xcompat64_map(u32 xid);
int xcompat64_unmap(u32 xid);
int xcompat64_mapped(u32 xid);
/* --- 55C: input v2 --- */
#define INPUTV264_KBD 1
#define INPUTV264_PTR 2
#define INPUTV264_TOUCH 4
int inputv264_add(int caps);
int inputv264_emit(int dev, int type, int code, int value, u64 time);
int inputv264_poll(int *dev, int *type, int *code, int *value);
int inputv264_grab(int dev);
/* --- 55D: output v2 --- */
int outputv264_add(const char *name);
int outputv264_mode(int id, u32 w, u32 h, u32 refresh);
int outputv264_layout(int id, int x, int y, int scale);
int outputv264_enable(int id, int on);
int outputv264_list(int *ids, int max);
/* --- 55E: virtual keyboard --- */
int virtkey64_press(u32 keycode);
int virtkey64_release(u32 keycode);
u32 virtkey64_mods(void);
int virtkey64_read(u32 *keycode, int *pressed);
/* --- 55F: screen sharing --- */
int screenshare64_start(int output, u32 fps);
int screenshare64_frame(int id, unsigned char *buf, u64 max);
int screenshare64_stop(int id);
int screenshare64_subscribers(int id);
/* --- 55G: remote desktop --- */
int remote64_hello(const char *client, u32 w, u32 h);
int remote64_handshake(int id, const char *token);
int remote64_input(int id, int type, int code, int value);
int remote64_update(int id, int *rects, int max);
int remote64_disconnect(int id);
/* --- 55H: security context --- */
#define SECCTX64_SCREENCAST 1
#define SECCTX64_CLIPBOARD 2
#define SECCTX64_INPUT 4
int secctx64_register(const char *appid, int sandbox);
int secctx64_grant(const char *appid, int perm);
int secctx64_revoke(const char *appid, int perm);
int secctx64_check(const char *appid, int perm);
int secctx64_prompt(const char *appid, int perm);
/* --- 55I: low latency path --- */
int lowlatpath64_scanout(u32 w, u32 h, u32 format);
int lowlatpath64_flip(int id);
u64 lowlatpath64_vblank(int id);
u32 lowlatpath64_latency_us(int id);
/* --- 56A: panel --- */
#define PANEL64_TOP 0
#define PANEL64_BOTTOM 1
#define PANEL64_LEFT 2
#define PANEL64_RIGHT 3
int panel64_create(int pos, int size);
int panel64_add_plugin(int panel, const char *name, int expand);
int panel64_remove_plugin(int panel, const char *name);
int panel64_geometry(int panel, int *x, int *y, int *w, int *h);
int panel64_autohide(int panel, int on);
int panel64_move_plugin(int panel, const char *name, int pos);
int panel64_plugin_count(int panel);
int panel64_struts(int panel, int *left, int *right, int *top,
                   int *bottom);
int panel64_opacity(int panel, int level);
int panel64_opacity_get(int panel);
int panel64_output(int panel, int output);
int panel64_output_get(int panel);
int panel64_plugin_at(int panel, int idx, char *out, int max);
/* --- 56B: pencere yoneticisi --- */
int wmde64_open(const char *title, int x, int y, int w, int h);
int wmde64_close(int id);
int wmde64_focus(int id);
int wmde64_focused(void);
int wmde64_minimize(int id, int on);
int wmde64_maximize(int id, int on);
int wmde64_move(int id, int x, int y);
int wmde64_resize(int id, int w, int h);
int wmde64_workspace(int ws);
int wmde64_current_workspace(void);
int wmde64_raise(int id);
int wmde64_lower(int id);
int wmde64_stack(int ws, int *out, int max);
int wmde64_sticky(int id, int on);
int wmde64_fullscreen(int id, int on);
int wmde64_constrain(int id, int minw, int minh, int maxw, int maxh);
int wmde64_geom(int id, int *x, int *y, int *w, int *h);
int wmde64_count(void);
/* --- 56C: ayar yoneticisi --- */
int settings64_set(const char *channel, const char *key, const char *val);
int settings64_get(const char *channel, const char *key, char *out,
                   int max);
int settings64_set_int(const char *channel, const char *key, long long v);
int settings64_get_int(const char *channel, const char *key,
                       long long *out);
int settings64_watch(const char *channel, const char *key);
int settings64_changed(char *channel_out, int cmax, char *key_out, int kmax);
int settings64_set_bool(const char *channel, const char *key, int v);
int settings64_get_bool(const char *channel, const char *key, int *out);
int settings64_set_double(const char *channel, const char *key, long long milli);
int settings64_get_double(const char *channel, const char *key, long long *milli_out);
int settings64_reset(const char *channel, const char *key);
int settings64_keys(const char *channel, char out[][48], int max);
int settings64_channels(char out[][32], int max);
int settings64_default(const char *channel, const char *key, const char *val);
int settings64_export(char *out, int max);
int settings64_import(const char *buf);
/* --- 56D: tema --- */
int theme64_add(const char *name);
int theme64_set_prop(const char *theme, const char *key, const char *val);
int theme64_get_prop(const char *theme, const char *key, char *out,
                     int max);
int theme64_apply(const char *name);
int theme64_active(char *out, int max);
u32 theme64_color(const char *hex);
int theme64_inherit(const char *child, const char *parent);
int theme64_variant(const char *theme, int dark);
int theme64_is_dark(const char *theme);
u32 theme64_lighten(u32 color, int amt);
u32 theme64_darken(u32 color, int amt);
u32 theme64_blend(u32 a, u32 b, int t);
u64 theme64_contrast(u32 fg, u32 bg);
int theme64_list(char out[][32], int max);
int theme64_remove_prop(const char *theme, const char *key);
int theme64_copy(const char *src, const char *dst);
/* --- 56E: dosya yoneticisi --- */
int fileman64_add(const char *path, int is_dir, u64 size);
int fileman64_list(const char *dir, char out[][64], int max);
int fileman64_select(const char *path);
int fileman64_clip_add(const char *path, int cut);
int fileman64_clip_paste(const char *dir);
int fileman64_mkdir(const char *path);
int fileman64_stat(const char *path, int *is_dir, u64 *size);
int fileman64_rename(const char *oldp, const char *newp);
int fileman64_remove(const char *path, int recursive);
int fileman64_copy(const char *src, const char *dst);
int fileman64_search(const char *dir, const char *substr, char out[][64],
                     int max);
int fileman64_sort(int mode);
u64 fileman64_dirsize(const char *dir);
int fileman64_touch(const char *path, u64 mtime);
/* --- 56F: baslat menusu --- */
int launcher64_add(const char *name, const char *exec, const char *icon,
                   const char *cats);
int launcher64_query(const char *cat, char out[][48], int max);
int launcher64_launch(const char *name, char *exec_out, int max);
int launcher64_fav(const char *name, int on);
int launcher64_remove(const char *name);
int launcher64_runs(const char *name);
int launcher64_top(int n, char out[][48], int max);
int launcher64_launch_argv(const char *name, const char *arg,
                           char *exec_out, int max);
int launcher64_hide(const char *name, int on);
int launcher64_icon(const char *name, char *out, int max);
int launcher64_cats(char out[][32], int max);
/* --- 56G: bildirim --- */
int notifyd64_send(const char *app, const char *summary, const char *body,
                   int timeout_ms);
int notifyd64_action(int id, const char *key);
int notifyd64_close(int id);
int notifyd64_expire(u64 now_ms);
int notifyd64_list(int *ids, int max);
int notifyd64_add_action(int id, const char *key);
int notifyd64_set_urgency(int id, int level);
int notifyd64_urgency(int id);
int notifyd64_get(int id, char *app_out, int amax, char *sum_out, int smax,
                  char *body_out, int bmax);
int notifyd64_replace(int id, const char *summary, const char *body);
int notifyd64_count(void);
int notifyd64_clear_all(void);
/* --- 56H: gorev cubugu --- */
int taskbar64_add(int win, const char *title);
int taskbar64_remove(int win);
int taskbar64_urgent(int win, int on);
int taskbar64_click(int win);
int taskbar64_clock(int hh, int mm, char *out, int max);
int taskbar64_list(int *wins, int max);
int taskbar64_title(int win, char *out, int max);
int taskbar64_minimized(int win);
int taskbar64_move(int win, int pos);
int taskbar64_pin(int win, int on);
int taskbar64_pinned(int win);
int taskbar64_clock12(int hh, int mm, char *out, int max);
/* --- 56I: sag-tik menus --- */
int ctxmenu64_create(void);
int ctxmenu64_add_item(int menu, const char *label, int action);
int ctxmenu64_add_submenu(int menu, const char *label, int sub);
int ctxmenu64_popup(int menu, int x, int y);
int ctxmenu64_activate(int menu, int index);
int ctxmenu64_remove_item(int menu, int idx);
int ctxmenu64_set_sensitive(int menu, int idx, int on);
int ctxmenu64_label(int menu, int idx, char *out, int max);
int ctxmenu64_action(int menu, int idx, int *out);
int ctxmenu64_count(int menu);
int ctxmenu64_close(int menu);
int ctxmenu64_pos(int menu, int *x, int *y);
int ctxmenu64_clear(int menu);
/* --- 57A: game launcher --- */
int gamed64_add(const char *name, const char *exec, const char *icon, const char *genre);
int gamed64_query(const char *genre, char out[][48], int max);
int gamed64_launch(const char *name, char *exec_out, int max);
int gamed64_fav(const char *name, int on);
int gamed64_remove(const char *name);
int gamed64_playtime(const char *name, u64 *out);
int gamed64_set_playtime(const char *name, u64 mins);
int gamed64_lastplayed(const char *name, u64 *out);
int gamed64_set_lastplayed(const char *name, u64 ts);
int gamed64_top(int n, char out[][48], int max);
int gamed64_cover(const char *name, char *out, int max);
/* --- 57B: controller mapping --- */
int controller64_add(int id, const char *name);
int controller64_map(int id, int button, int action);
int controller64_get_mapping(int id, int button, int *action_out);
int controller64_remove_mapping(int id, int button);
int controller64_deadzone(int id, int dz);
int controller64_rumble(int id, int left, int right);
int controller64_profile_create(int id, const char *name);
int controller64_profile_apply(int id, const char *name);
int controller64_list(int *ids, int max);
/* --- 57C: FPS overlay --- */
int fps64_start(void);
int fps64_stop(void);
int fps64_tick(u64 now_ms);
int fps64_fps(u64 *out);
int fps64_overlay(int on);
int fps64_pos(int x, int y);
int fps64_get_pos(int *x, int *y);
int fps64_set_format(int fmt);
int fps64_format(int *out);
/* --- 57D: performans HUD --- */
int perf64_init(void);
int perf64_shutdown(void);
int perf64_update_cpu(u64 pct);
int perf64_update_ram(u64 used, u64 total);
int perf64_get_cpu(u64 *out);
int perf64_get_ram_used(u64 *out);
int perf64_get_ram_total(u64 *out);
int perf64_overlay(int on);
int perf64_pos(int x, int y);
int perf64_get_pos(int *x, int *y);
int perf64_refresh(int ms);
int perf64_get_refresh(int *out);
/* --- 57E: replay sistem --- */
int replay64_start(const char *name);
int replay64_stop(void);
int replay64_record_start(void);
int replay64_record_stop(void);
int replay64_record_frame(int frame);
int replay64_is_recording(void);
int replay64_save(const char *name);
int replay64_load(const char *name);
int replay64_quality(int q);
int replay64_get_frame_count(int *out);
int replay64_playback_start(const char *name);
int replay64_playback_stop(void);
/* --- 57F: achievement sistemi --- */
int achievement64_add(int id, const char *name, const char *desc);
int achievement64_unlock(int id);
int achievement64_is_unlocked(int id);
int achievement64_progress(int id, int pct);
int achievement64_get_progress(int id, int *out);
int achievement64_count(int *out);
int achievement64_list_ids(int *ids, int max);
/* --- 57G: mod destek --- */
int mod64_load(const char *name, const char *path);
int mod64_unload(int id);
int mod64_enable(int id);
int mod64_disable(int id);
int mod64_verify(int id);
int mod64_is_enabled(int id);
int mod64_count(int *out);
int mod64_list_ids(int *ids, int max);
/* --- 57H: cloud save --- */
int cloud64_save(const char *game, int slot, const char *data);
int cloud64_load(const char *game, int slot, char *out, int max);
int cloud64_sync(const char *game, int slot);
int cloud64_status(const char *game, int slot, int *out);
int cloud64_delete(const char *game, int slot);
/* --- 57I: anti-cheat --- */
int anticheat64_init(void);
int anticheat64_scan(int pid);
int anticheat64_report(int pid, const char *reason);
int anticheat64_ban(int pid);
int anticheat64_is_banned(int pid);
/* --- 57J: game test marker --- */
/* --- 58A: user namespaces --- */
/* uses existing userns64_add_map / userns64_map_uid / userns64_level / userns64_enter */
/* --- 58B: seccomp filtre --- */
int seccomp64_load(void);
int seccomp64_add_rule(int nr, int action);
int seccomp64_check(int nr);
int seccomp64_unload(void);
/* --- 58C: mount namespace --- */
/* uses existing mountns64_mount / mountns64_unmount / mountns64_resolve */
/* --- 58D: PID namespace --- */
int pidns64_create(int *out_id);
int pidns64_destroy(int id);
int pidns64_count(int *out);
/* --- 58E: network namespace --- */
int netns64_create(int *out_id);
int netns64_destroy(int id);
int netns64_count(int *out);
/* --- 58F: cgroups v2 --- */
int cgroup64_create(const char *name,int *out);
int cgroup64_destroy(int id);
int cgroup64_count(int *out);
/* --- 58G: AppArmor profil --- */
int apparmor64_load(const char *name,int *out);
int apparmor64_unload(int id);
int apparmor64_count(int *out);
/* --- 58H: Flatpak manifest --- */
int flatpak64_parse(const char *manifest,int *out);
int flatpak64_remove(int id);
int flatpak64_count(int *out);
/* --- 58I: runtime izolasyon --- */
int runtime64_start(const char *name,int *out);
int runtime64_stop(int id);
int runtime64_count(int *out);
/* --- 59A: Live ISO 64-bit --- */
int liveiso64_create(const char *path,int *out);
int liveiso64_destroy(int id);
int liveiso64_count(int *out);
/* --- 59B: Kurulum sihirbazi --- */
int installer64_start(int *out);
int installer64_stop(int id);
int installer64_count(int *out);
/* --- 59C: Partition wizard --- */
int partition64_new(int *out);
int partition64_del(int id);
int partition64_count(int *out);
/* --- 59D: LUKS setup --- */
/* uses existing luks64_format / luks64_add_key / luks64_unlock / luks64_lock */
/* --- 59E: Bootloader --- */
int bootloader64_install(int *out);
int bootloader64_remove(int id);
int bootloader64_count(int *out);
/* --- 59F: Network install --- */
int netinstall64_start(int *out);
int netinstall64_stop(int id);
int netinstall64_count(int *out);
/* --- 59G: PXE boot --- */
int pxe64_start(int *out);
int pxe64_stop(int id);
int pxe64_count(int *out);
/* --- 59H: Auto-install --- */
int autoinstall64_start(int *out);
int autoinstall64_stop(int id);
int autoinstall64_count(int *out);
/* --- 59I: Recovery ISO --- */
int recovery64_create(int *out);
int recovery64_destroy(int id);
int recovery64_count(int *out);
/* --- 60A: Release engineering --- */
int release64_create(int *out);
int release64_destroy(int id);
int release64_count(int *out);
/* --- 60B: Changelog --- */
int changelog64_create(int *out);
int changelog64_destroy(int id);
int changelog64_count(int *out);
/* --- 60C: Man pages --- */
int manpages64_create(int *out);
int manpages64_destroy(int id);
int manpages64_count(int *out);
/* --- 60D: API docs --- */
int apidocs64_create(int *out);
int apidocs64_destroy(int id);
int apidocs64_count(int *out);
/* --- 60E: Website --- */
int website64_create(int *out);
int website64_destroy(int id);
int website64_count(int *out);
/* --- 60F: CI/CD pipeline --- */
int cicd64_create(int *out);
int cicd64_destroy(int id);
int cicd64_count(int *out);
/* --- 60G: Paket deposu --- */
int paketdepo64_create(int *out);
int paketdepo64_destroy(int id);
int paketdepo64_count(int *out);
/* --- 60H: Security advisory --- */
int security64_create(int *out);
int security64_destroy(int id);
int security64_count(int *out);
/* --- 60I: LTS branch --- */
int lts64_create(int *out);
int lts64_destroy(int id);
int lts64_count(int *out);
/* --- 60J: charOS v1.0 release --- */
int releasev1_64_create(int *out);
int releasev1_64_destroy(int id);
int releasev1_64_count(int *out);
/* --- 36I: jemalloc tarzi API --- */
#define JEM64_ZERO (1 << 6)
void *jem64_mallocx(u64 size, int flags); /* flags: dusuk 6 bit=lg_align */
void *jem64_rallocx(void *p, u64 oldsize, u64 size, int flags);
u64 jem64_xallocx(void *p, u64 oldsize, u64 size); /* yerinde buyume */
u64 jem64_sallocx(u64 size);
void jem64_dallocx(void *p, u64 size);
u64 jem64_nallocx(u64 size);

/* --- 1A: Secure Boot Design --- */
typedef struct {
    u8 hash[32];
    u64 pcr_index;
    u64 timestamp;
    u8 event_type;
} secureboot_event_t;
typedef enum {
    SB_OK = 0,
    SB_ERR_PARAM = -1,
    SB_ERR_INVALID_SIG = -2,
    SB_ERR_CRYPTO = -3,
} secureboot_err_t;
int secureboot64_init(void);
int secureboot64_get_pcr(u8 *out, u64 len);

/* --- 2A: TPM 2.0 --- */
int tpm64_init(void);
int tpm64_extend_pcr(u64 pcr_idx, const u8 *hash, u64 hash_len);
int tpm64_read_pcr(u64 pcr_idx, u8 *out);
int tpm64_quote(u8 *quote, u64 *len);
int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len);
int secureboot64_measure_kernel(const void *kernel, u64 size);
void secureboot64_log_event(const char *event);
int secureboot64_get_last_event(secureboot_event_t *out);

/* --- 4A: Advanced Memory (davranis modeli) --- */
int advmem64_init(void);
int advmem64_alloc_node(int node, int order, u64 *out_phys);
int advmem64_map_huge(u64 virt, u64 phys, u64 flags, int level);
void advmem64_kasan_poison(const void *addr, u64 size);
int advmem64_kasan_check(const void *addr, u64 size);
int advmem64_reclaim(u64 target_pages, u64 *freed);

/* --- 5A: Process & Threading (davranis modeli) --- */
int proc64_init(void);
int proc64_spawn(u64 *out_pid);
int proc64_exit(u64 pid, int code);
int proc64_wait(u64 pid, int *out_code);
int proc64_state(u64 pid, int *out_state);

#endif
