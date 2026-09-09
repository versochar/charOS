#ifndef CHAROS_DRIVERS_RTC_H
#define CHAROS_DRIVERS_RTC_H

#include <stdint.h>

struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year; // full year e.g. 2026
    uint8_t weekday; // 1=Sunday
};

void rtc_init(void);
void rtc_get_time(struct rtc_time* t);
void rtc_dump(void);
/* 14G: RTC alarm (saniye/dakika/saat eşleşmesi, anketli) */
void rtc_alarm_in(int seconds_ahead); /* 0-59 sn sonrası */
int rtc_alarm_fired(void);            /* 1 çaldı (bayrağı temizler) */

#endif