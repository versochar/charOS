#include <drivers/keyboard.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/input.h>
#include <drivers/vt.h>
#include <drivers/inr.h>
#include <core/pic.h>
#include <core/isr.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Ring buffer 256 byte */
#define KBD_BUF_SIZE 256
static char kbd_buffer[KBD_BUF_SIZE];
static volatile uint16_t kbd_head = 0;
static volatile uint16_t kbd_tail = 0;
static volatile uint8_t shift_pressed = 0;
static volatile uint8_t caps_lock = 0;
static volatile uint8_t ctrl_pressed = 0;
static volatile uint8_t alt_pressed = 0;
static volatile uint8_t extended = 0;

/* US scancode -> ASCII (unshifted) */
static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0, 0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Shift ile beraber */
static const char scancode_shift_map[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0,
    '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0, 0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static void kbd_buffer_put(char c) {
    uint16_t next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next == kbd_tail) {
        /* Buffer dolu - en eskiyi atma, yeni karakteri at */
        return;
    }
    kbd_buffer[kbd_head] = c;
    kbd_head = next;
}

static char kbd_buffer_get(void) {
    if (kbd_head == kbd_tail) return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}

void keyboard_flush(void) {
    asm volatile("cli");
    kbd_head = kbd_tail = 0;
    asm volatile("sti");
}

int keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}

char keyboard_getchar_nonblock(void) {
    asm volatile("cli");
    char c = kbd_buffer_get();
    asm volatile("sti");
    return c;
}

char keyboard_getchar(void) {
    while (!keyboard_has_char()) {
        // schedule'dan dönüşte IF kapalı kalabilir (syscall/IRQ gate IF=0);
        // hlt öncesi sti şart, yoksa timer giremez ve CPU sonsuz uyur.
        asm volatile("sti; hlt");
    }
    asm volatile("cli");
    char c = kbd_buffer_get();
    asm volatile("sti");
    return c;
}

void keyboard_handler(struct registers* regs)
{
    (void)regs;
    uint8_t scancode = inb(KBD_DATA_PORT);
    keyboard_scancode(scancode);
}

/* 26C: PS/2 bayt işleme (USB-HID de aynı yolu kullanır: press=kod,
 * release=kod|0x80, oklar=0xE0 öneki). IRQ dısı çağrılabilir. */
void keyboard_scancode(uint8_t scancode)
{
    /* Extended prefix */
    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    uint8_t released = scancode & 0x80;
    uint8_t key = scancode & 0x7F;

    if (extended) {
        /* 19J: ok tuşları ANSI kaçış dizisi üretir (xterm konvansiyonu).
         * Tek-alıcı kuralı normal karakterlerle aynıdır. */
        extended = 0;
        if (!released) {
            char seq = 0;
            if (key == 0x48) seq = 'A';      /* yukarı */
            else if (key == 0x50) seq = 'B'; /* aşağı */
            else if (key == 0x4B) seq = 'D'; /* sol */
            else if (key == 0x4D) seq = 'C'; /* sag */
            if (seq) {
                if (vt_gui_active() && !inr_claims_keyboard()) {
                    kbd_buffer_put(KEY_ESC); kbd_buffer_put('['); kbd_buffer_put(seq);
                }
            }
        }
        return;
    }

    if (released) {
        /* Tuş bırakıldı */
        switch (key) {
            case 0x2A: /* Left Shift */
            case 0x36: /* Right Shift */
                shift_pressed = 0; break;
            case 0x1D: /* Ctrl */
                ctrl_pressed = 0; break;
            case 0x38: /* Alt */
                alt_pressed = 0; break;
            default: break;
        }
        input_key(key, 0); /* 18A: hub'a bırakma olayı */
        return;
    }

    /* Tuş basıldı */
    input_key(key, 1); /* 18A: hub'a basma olayı (modifier dahil) */
    switch (key) {
        case 0x2A: case 0x36: /* Shift */
            shift_pressed = 1; return;
        case 0x1D:
            ctrl_pressed = 1; return;
        case 0x38:
            alt_pressed = 1; return;
        case 0x3A: /* Caps Lock */
            caps_lock = !caps_lock; return;
        default: break;
    }

    char c = 0;
    if (key < 128) {
        int shifted = shift_pressed ^ caps_lock; /* caps + shift = ters */
        /* Harfler için caps/shift işlemi, diğerleri için sadece shift */
        if (scancode_map[key] >= 'a' && scancode_map[key] <= 'z') {
            c = shifted ? scancode_shift_map[key] : scancode_map[key];
        } else if (scancode_map[key] >= 'A' && scancode_map[key] <= 'Z') {
            c = shifted ? scancode_shift_map[key] : scancode_map[key];
        } else {
            c = shift_pressed ? scancode_shift_map[key] : scancode_map[key];
        }
    }

    if (c) {
        /* Kısıt-2 düzeltmesi: tek alıcı kuralı (çift yankı yok).
         * Konsol modunda (text VT aktif) klavye doğrudan çekirdek tamponuna
         * gider ve userland kabuğu (sh) okur. GUI modunda hub->inr terminali
         * yolunda; inr odağı yoksa yine çekirdek tamponu. */
        if (vt_gui_active() && !inr_claims_keyboard())
            kbd_buffer_put(c);
        /* GUI odağı inr'deyse hub üzerinden terminale gider, tampona yazılmaz */
    } else {
        /* Bilinmeyen tuş - esc gibi özel (tek alıcı kuralı burada da geçerli) */
        if (key == 0x01) { /* ESC */
            if (vt_gui_active() && !inr_claims_keyboard())
                kbd_buffer_put(KEY_ESC);
        }
    }
}

void keyboard_init(void)
{
    /* IRQ1 handler'ı kaydet (int 33) */
    isr_register_handler(33, keyboard_handler);

    /* Klavye buffer'ı temizle */
    kbd_head = kbd_tail = 0;
    shift_pressed = 0; caps_lock = 0;
    extended = 0;

    /* Klavye controller'ı flush et (bekleyen veri varsa) */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }

    /* PIC'de IRQ1 maskesini kaldır */
    pic_clear_mask(1);

    serial_puts("[2B] Keyboard init (IRQ1) done\n");
}