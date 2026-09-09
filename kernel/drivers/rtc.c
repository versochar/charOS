#include <drivers/rtc.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

static inline void outb(uint16_t p, uint8_t v){ asm volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; asm volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static int is_updating(void) {
    outb(0x70, 0x0A);
    uint8_t v = inb(0x71);
    if (v == 0xFF) return 0; /* RTC portu yok/floating bus */
    return v & 0x80;
}

static void wait_rtc_ready(void) {
    for (volatile int i = 0; i < 100000; i++) {
        if (!is_updating()) break;
        if ((i & 0xFF) == 0) asm volatile("pause");
    }
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

void rtc_init(void) {
    // RTC zaten çalışıyor, sadece status kontrol
    serial_puts("[5A] RTC init\n");
}

void rtc_get_time(struct rtc_time* t) {
    uint8_t sec, min, hour, day, mon, year, weekday, century;
    uint8_t regB;

    // Update bit bekle (zaman aşımı ile)
    wait_rtc_ready();

    sec = rtc_read(0x00);
    min = rtc_read(0x02);
    hour = rtc_read(0x04);
    weekday = rtc_read(0x06);
    day = rtc_read(0x07);
    mon = rtc_read(0x08);
    year = rtc_read(0x09);
    // Century varsa 0x32'de, yoksa 20 varsay
    century = rtc_read(0x32);
    regB = rtc_read(0x0B);

    // BCD -> binary eğer BCD modda ise (regB bit2 = 0)
    if (!(regB & 0x04)) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour & 0x7F) | (hour & 0x80); // 12h bit koru
        weekday = bcd_to_bin(weekday);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        year = bcd_to_bin(year);
        if (century != 0xFF) century = bcd_to_bin(century);
    }

    // 12 saat modunu 24'e çevir
    if (!(regB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }
    hour &= 0x7F;

    // Century düzelt
    uint16_t full_year;
    if (century == 0 || century == 0xFF || century > 99) {
        // Century register yok veya geçersiz, 20xx varsay
        full_year = 2000 + year;
        // Eğer year < 70 ve century yoksa 2000+ , değilse 1900+ ?
        // Basit: 2000+year, ama 2026 için year=26 => 2026 doğru
        if (full_year < 2020 && year > 90) full_year = 1900 + year;
    } else {
        full_year = century * 100 + year;
    }

    t->second = sec;
    t->minute = min;
    t->hour = hour;
    t->day = day;
    t->month = mon;
    t->year = full_year;
    t->weekday = weekday;
}

static void rtc_write(uint8_t reg, uint8_t val) {
    outb(0x70, reg);
    outb(0x71, val);
}

static uint8_t to_bcd_if(uint8_t v, int binary_mode) {
    if (binary_mode) return v;
    return (uint8_t)(((v / 10) << 4) | (v % 10));
}

/* 14G: seconds_ahead sonrası çalacak alarm kur (tarih taşması yok sayılır) */
void rtc_alarm_in(int seconds_ahead) {
    struct rtc_time t;
    if (seconds_ahead < 0) seconds_ahead = 0;
    wait_rtc_ready();
    rtc_get_time(&t);
    int s = t.second + seconds_ahead;
    int m = t.minute, h = t.hour;
    if (s >= 60) { s -= 60; m++; if (m >= 60) { m -= 60; h = (h + 1) % 24; } }
    uint8_t regB = rtc_read(0x0B);
    int bin = (regB & 0x04) ? 1 : 0;
    int h24 = (regB & 0x02) ? 1 : 0;
    uint8_t ah = (uint8_t)h;
    if (!h24) {
        /* 12 saat modu: PM biti + 1-12 aralığı */
        if (ah == 0) ah = 12;
        else if (ah > 12) { ah -= 12; }
        if (t.hour >= 12 && ah <= 12) ah |= 0x80;
    }
    rtc_write(0x01, to_bcd_if((uint8_t)s, bin));
    rtc_write(0x03, to_bcd_if((uint8_t)m, bin));
    rtc_write(0x05, to_bcd_if(ah, bin));
    rtc_write(0x0B, regB | 0x20); /* alarm kesme izni (anket bayrağı için) */
    (void)rtc_read(0x0C); /* bekleyen bayrağı temizle */
}

/* 14G: AF bayrağı geldiyse 1 dön + temizle */
int rtc_alarm_fired(void) {
    outb(0x70, 0x0C);
    uint8_t c = inb(0x71);
    return (c & 0x20) ? 1 : 0;
}

void rtc_dump(void) {
    struct rtc_time t;
    rtc_get_time(&t);
    vga_puts("Date: "); vga_putdec(t.day); vga_putc('/'); vga_putdec(t.month); vga_putc('/'); vga_putdec(t.year);
    vga_puts(" Time: "); vga_putdec(t.hour); vga_putc(':'); vga_putdec(t.minute); vga_putc(':'); vga_putdec(t.second);
    vga_puts(" WD:"); vga_putdec(t.weekday); vga_puts("\n");
    serial_puts("Date: "); serial_puts(""); // serial dec manual
    // Serial için de yaz
    serial_puts("RTC dump done\n");
}