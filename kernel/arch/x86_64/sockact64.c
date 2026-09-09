/* 42C: socket activation — ilk baglantida servis uyandirma + kuyruk. */
#include "arch/x86_64/longmode.h"

#define SOCKACT64_MAX 16
#define SOCKACT64_QUEUE 32

struct sockact64_entry {
    int used;
    char sock[64];
    char service[32];
};

struct sockact64_pending {
    char sock[64];
};

static struct sockact64_entry sockact64_tab[SOCKACT64_MAX];
static struct sockact64_pending sockact64_q[SOCKACT64_QUEUE];
static int sockact64_n = 0;

static void sock_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int sock_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int sockact64_add(const char *sock, const char *service) {
    int i;
    if (!sock || !service) return -1;
    for (i = 0; i < SOCKACT64_MAX; i++) {
        if (!sockact64_tab[i].used) {
            sockact64_tab[i].used = 1;
            sock_str_copy(sockact64_tab[i].sock, sock, 64);
            sock_str_copy(sockact64_tab[i].service, service, 32);
            return 0;
        }
    }
    return -2;
}

/* Baglanti: servisi dondur + kuyruga al. 0=tetiklendi, <0 eslesme yok. */
int sockact64_connection(const char *sock, char *service_out, int max) {
    int i;
    if (!sock) return -1;
    for (i = 0; i < SOCKACT64_MAX; i++) {
        if (!sockact64_tab[i].used) continue;
        if (!sock_str_eq(sockact64_tab[i].sock, sock)) continue;
        if (service_out && max > 0)
            sock_str_copy(service_out, sockact64_tab[i].service, max);
        if (sockact64_n < SOCKACT64_QUEUE)
            sock_str_copy(sockact64_q[sockact64_n++].sock, sock, 64);
        return 0;
    }
    return -2;
}

int sockact64_pending(void) { return sockact64_n; }
