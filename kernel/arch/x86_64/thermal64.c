/* 48H: thermal trip — bolge sicakligi + esik + sogutma kademesi. */
#include "arch/x86_64/longmode.h"

#define THERMAL64_MAX_ZONES 8

struct thermal64_zone {
    int used;
    int temp_c;
    int passive_trip;
    int critical_trip;
    int cooling;
};

static struct thermal64_zone thermal64_tab[THERMAL64_MAX_ZONES];

static void thermal64_ensure(int zone) {
    if (zone < 0 || zone >= THERMAL64_MAX_ZONES) return;
    if (!thermal64_tab[zone].used) {
        thermal64_tab[zone].used = 1;
        thermal64_tab[zone].temp_c = 40;
        thermal64_tab[zone].passive_trip = 85;
        thermal64_tab[zone].critical_trip = 100;
        thermal64_tab[zone].cooling = 0;
    }
}

int thermal64_set_temp(int zone, int temp_c) {
    if (zone < 0 || zone >= THERMAL64_MAX_ZONES) return -1;
    thermal64_ensure(zone);
    thermal64_tab[zone].temp_c = temp_c;
    return 0;
}

/* 0=normal, 1=pasif (kis), 2=kritik (kapat). */
int thermal64_check(int zone) {
    if (zone < 0 || zone >= THERMAL64_MAX_ZONES) return -1;
    thermal64_ensure(zone);
    if (thermal64_tab[zone].temp_c >= thermal64_tab[zone].critical_trip)
        return 2;
    if (thermal64_tab[zone].temp_c >= thermal64_tab[zone].passive_trip)
        return 1;
    return 0;
}

int thermal64_cooling(int zone, int level) {
    if (zone < 0 || zone >= THERMAL64_MAX_ZONES) return -1;
    if (level < 0 || level > 10) return -1;
    thermal64_ensure(zone);
    thermal64_tab[zone].cooling = level;
    return 0;
}
