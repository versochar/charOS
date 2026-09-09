/* 46E: JACK skeleton — portlar + baglantilar + dongu sayaci. */
#include "arch/x86_64/longmode.h"

#define JACK64_MAX_PORTS 32
#define JACK64_MAX_CONN 32

struct jack64_port {
    int used;
    char name[32];
    int type;
    int is_input;
};

struct jack64_conn {
    int src;
    int dst;
};

static struct jack64_port jack64_ports[JACK64_MAX_PORTS];
static struct jack64_conn jack64_conns[JACK64_MAX_CONN];
static int jack64_nconn = 0;
static u32 jack64_rate = 48000;
static u32 jack64_bufsize = 1024;
static u64 jack64_cycles = 0;

static void jack_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int jack64_set_rate(u32 rate, u32 bufsize) {
    if (!rate || !bufsize) return -1;
    jack64_rate = rate;
    jack64_bufsize = bufsize;
    return 0;
}

int jack64_port_register(const char *name, int type, int is_input) {
    int i;
    if (!name || (type != JACK64_AUDIO && type != JACK64_MIDI)) return -1;
    for (i = 0; i < JACK64_MAX_PORTS; i++) {
        if (!jack64_ports[i].used) {
            jack64_ports[i].used = 1;
            jack_str_copy(jack64_ports[i].name, name, 32);
            jack64_ports[i].type = type;
            jack64_ports[i].is_input = is_input ? 1 : 0;
            return i;
        }
    }
    return -2;
}

static int jack64_valid(int p) {
    return p >= 0 && p < JACK64_MAX_PORTS && jack64_ports[p].used;
}

int jack64_connect(int src, int dst) {
    int i;
    if (!jack64_valid(src) || !jack64_valid(dst)) return -1;
    if (jack64_ports[src].is_input || !jack64_ports[dst].is_input)
        return -2; /* yon: cikis -> giris */
    if (jack64_ports[src].type != jack64_ports[dst].type) return -3;
    for (i = 0; i < jack64_nconn; i++) {
        if (jack64_conns[i].src == src && jack64_conns[i].dst == dst)
            return 0;
    }
    if (jack64_nconn >= JACK64_MAX_CONN) return -4;
    jack64_conns[jack64_nconn].src = src;
    jack64_conns[jack64_nconn].dst = dst;
    jack64_nconn++;
    return 0;
}

int jack64_cycle(void) {
    jack64_cycles++;
    return jack64_nconn; /* islenen baglanti sayisi */
}
