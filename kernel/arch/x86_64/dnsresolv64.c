/* 51C: DNS resolver — sunucu/arama/hosts + onbellek cozumu. */
#include "arch/x86_64/longmode.h"

#define DNSRESOLV64_MAX_NS 4
#define DNSRESOLV64_MAX_SEARCH 4
#define DNSRESOLV64_MAX_HOSTS 32

static char dnsresolv64_ns[DNSRESOLV64_MAX_NS][64];
static int dnsresolv64_nns = 0;
static char dnsresolv64_domains[DNSRESOLV64_MAX_SEARCH][64];
static int dnsresolv64_nsearch = 0;

struct dnsresolv64_host {
    int used;
    char name[64];
    char ip[64];
};

static struct dnsresolv64_host dnsresolv64_hosts[DNSRESOLV64_MAX_HOSTS];

static void dnsr_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int dnsr_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        if (!ca) return 1;
    }
}

static int dnsr_has_dot(const char *s) {
    int i;
    for (i = 0; s[i]; i++) {
        if (s[i] == '.') return 1;
    }
    return 0;
}

int dnsresolv64_nameserver(const char *ip) {
    if (!ip || dnsresolv64_nns >= DNSRESOLV64_MAX_NS) return -1;
    dnsr_str_copy(dnsresolv64_ns[dnsresolv64_nns++], ip, 64);
    return 0;
}

int dnsresolv64_search(const char *domain) {
    if (!domain || dnsresolv64_nsearch >= DNSRESOLV64_MAX_SEARCH)
        return -1;
    dnsr_str_copy(dnsresolv64_domains[dnsresolv64_nsearch++], domain, 64);
    return 0;
}

int dnsresolv64_add_host(const char *name, const char *ip) {
    int i;
    if (!name || !ip) return -1;
    for (i = 0; i < DNSRESOLV64_MAX_HOSTS; i++) {
        if (!dnsresolv64_hosts[i].used) {
            dnsresolv64_hosts[i].used = 1;
            dnsr_str_copy(dnsresolv64_hosts[i].name, name, 64);
            dnsr_str_copy(dnsresolv64_hosts[i].ip, ip, 64);
            return 0;
        }
    }
    return -2;
}

/* Cozum sirasi: hosts -> (noktaliysa) dns onbellegi -> arama eki.
 * Ag sorgusu 43F'te; burada onbellek + hosts. */
int dnsresolv64_lookup(const char *name, char *out, int max) {
    int i;
    char fqdn[128];
    if (!name || !out || max <= 0) return -1;
    for (i = 0; i < DNSRESOLV64_MAX_HOSTS; i++) {
        if (dnsresolv64_hosts[i].used &&
            dnsr_str_eq(dnsresolv64_hosts[i].name, name)) {
            dnsr_str_copy(out, dnsresolv64_hosts[i].ip, max);
            return 0;
        }
    }
    if (dnsr_has_dot(name)) {
        u32 ip = 0;
        if (dns64_cached(name, &ip) == 0) {
            if (ipstack64_fmt4(ip, out, max) == 0) return 0;
        }
        return -2; /* onbellek disi ag 43F'te */
    }
    for (i = 0; i < dnsresolv64_nsearch; i++) {
        int k = 0, j;
        for (j = 0; name[j] && k < 120; j++) fqdn[k++] = name[j];
        fqdn[k++] = '.';
        for (j = 0; dnsresolv64_domains[i][j] && k < 127; j++)
            fqdn[k++] = dnsresolv64_domains[i][j];
        fqdn[k] = 0;
        {
            u32 ip = 0;
            if (dns64_cached(fqdn, &ip) == 0) {
                if (ipstack64_fmt4(ip, out, max) == 0) return 0;
            }
        }
    }
    return -3;
}
