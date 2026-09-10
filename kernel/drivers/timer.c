#include <drivers/timer.h>
#include <process/task.h>
#include <core/power.h>
#include <drivers/thermal.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <core/pic.h>
#include <core/isr.h>

static volatile uint32_t tick = 0;
static void (*timer_callback)(void) = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

extern struct task* task_list;
extern struct task* current_task;

void timer_handler(struct registers* regs)
{
    (void)regs;
    tick++;
    power_note_tick(task_on_idle()); /* 31.4: güç muhasebesi (ucuz: 2 sayaç) */
    /* Tanılama nabzı (1Hz): PIT + RDTSC + LAPIC. Üç bağımsız saatin
     * birlikte okunması hangi katmanın aksadığını gösterir. */
    if ((tick % 100) == 0) {
        uint32_t lo = 0, hi = 0;
        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
        extern uint32_t lapic_tick_get(void);
        serial_puts("[HB] t=");
        serial_puthex(tick);
        serial_puts(" c=");
        serial_puthex(hi);
        serial_puthex(lo);
        serial_puts(" l=");
        serial_puthex(lapic_tick_get());
        serial_puts("\n");
        thermal_dynamic_idle(); /* 32.4: 1Hz termal örnekleme + regülatör */
    }
    /* 21A: ağ arayüzünden gelen paketleri al */
    extern void e1000_timer_poll(void);
    e1000_timer_poll();
    /* 26C: USB-HID kesme raporlarını tara (100Hz) */
    extern void usbhid_timer_poll(void);
    usbhid_timer_poll();
    // 12E: wakeup sleepers
    struct task* t = task_list;
    while (t) {
        if (t->state == TASK_BLOCKED && t->sleep_until && tick >= t->sleep_until) {
            t->sleep_until = 0;
            t->state = TASK_READY;
        }
        t = t->next;
        if (!t) break;
    }
    if (timer_callback) timer_callback();
}

void sleep_until(uint32_t until) {
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : current_task;
    if (!cur) return;
    cur->sleep_until = until;
    cur->state = TASK_BLOCKED;
    // will be woken by timer_handler
    schedule();
}

uint32_t timer_get_ticks(void) { return tick; }

void timer_wait(uint32_t ticks)
{
    uint32_t et = tick + ticks;
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    int irq_enabled = (eflags & 0x200) != 0;

    if (!irq_enabled) {
        /* Kesmeler kapalıyken hlt CPU'yu sonsuza kadar kilitler.
         * Basit busy-loop ile gecikme sağla. */
        volatile uint32_t count = ticks * 500000;
        for (volatile uint32_t i = 0; i < count; i++) asm volatile("pause");
        return;
    }

    while (tick < et) asm volatile("hlt");
}

/* Kısıt-dışı gerçek uyku: task'ı BLOCKED yapıp CPU'yu bırakır.
 * timer_wait hlt-spin yapar ama RUNNING kalır; sonsuz polling
 * döngülerinde dilim rotasyonunu şişirip giriş gecikmesi yapar. */
void task_sleep(uint32_t ticks)
{
    sleep_until(tick + ticks);
}

void timer_set_callback(void (*cb)(void)) { timer_callback = cb; }

void timer_init(uint32_t freq)
{
    if (freq == 0) freq = 100;
    uint32_t divisor = PIT_FREQ / freq;
    if (divisor == 0) divisor = 1;
    if (divisor > 65535) divisor = 65535;

    /* IRQ0 handler'ı kaydet (int 32) */
    isr_register_handler(32, timer_handler);

    /* PIT channel 0, lobyte/hibyte, mode 3 (square wave), binary */
    outb(PIT_CMD, 0x36);
    outb(PIT_DATA0, divisor & 0xFF);
    outb(PIT_DATA0, (divisor >> 8) & 0xFF);

    /* PIC'de IRQ0 maskesini kaldır */
    pic_clear_mask(0);

    tick = 0;
}