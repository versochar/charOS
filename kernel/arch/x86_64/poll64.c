/* 39G: poll/epoll — hazirlik tablosu + seviye-tetikli bekleme. */
#include "arch/x86_64/longmode.h"

#define POLL64_MAX_FD 64
#define EPOLL64_MAX 8
#define EPOLL64_MAX_EVENTS 32

static u32 poll64_ready[POLL64_MAX_FD];

struct epoll64_set {
    int used;
    int fds[EPOLL64_MAX_EVENTS];
    u32 events[EPOLL64_MAX_EVENTS];
    int n;
};

static struct epoll64_set epoll64_tab[EPOLL64_MAX];

int poll64_set(int fd, u32 revents) {
    if (fd < 0 || fd >= POLL64_MAX_FD) return -1;
    poll64_ready[fd] = revents;
    return 0;
}

int poll64_poll(const int *fds, int nfds, u32 events, u32 *revents_out) {
    int i, ready = 0;
    if (!fds || nfds <= 0) return -1;
    for (i = 0; i < nfds; i++) {
        u32 hit = 0;
        int fd = fds[i];
        if (fd < 0 || fd >= POLL64_MAX_FD) continue;
        hit = poll64_ready[fd] & events;
        if (revents_out) revents_out[i] = hit;
        if (hit) ready++;
    }
    return ready;
}

int epoll64_create(void) {
    int i;
    for (i = 0; i < EPOLL64_MAX; i++) {
        if (!epoll64_tab[i].used) {
            epoll64_tab[i].used = 1;
            epoll64_tab[i].n = 0;
            return i;
        }
    }
    return -1;
}

static struct epoll64_set *epoll64_get(int ep) {
    if (ep < 0 || ep >= EPOLL64_MAX || !epoll64_tab[ep].used) return 0;
    return &epoll64_tab[ep];
}

int epoll64_add(int ep, int fd, u32 events) {
    struct epoll64_set *s = epoll64_get(ep);
    int i;
    if (!s || fd < 0 || fd >= POLL64_MAX_FD) return -1;
    for (i = 0; i < s->n; i++)
        if (s->fds[i] == fd) return -2;
    if (s->n >= EPOLL64_MAX_EVENTS) return -3;
    s->fds[s->n] = fd;
    s->events[s->n] = events;
    s->n++;
    return 0;
}

int epoll64_mod(int ep, int fd, u32 events) {
    struct epoll64_set *s = epoll64_get(ep);
    int i;
    if (!s) return -1;
    for (i = 0; i < s->n; i++) {
        if (s->fds[i] == fd) {
            s->events[i] = events;
            return 0;
        }
    }
    return -2;
}

int epoll64_del(int ep, int fd) {
    struct epoll64_set *s = epoll64_get(ep);
    int i;
    if (!s) return -1;
    for (i = 0; i < s->n; i++) {
        int j;
        if (s->fds[i] != fd) continue;
        for (j = i; j + 1 < s->n; j++) {
            s->fds[j] = s->fds[j + 1];
            s->events[j] = s->events[j + 1];
        }
        s->n--;
        return 0;
    }
    return -2;
}

int epoll64_wait(int ep, int *fds_out, u32 *ev_out, int max) {
    struct epoll64_set *s = epoll64_get(ep);
    int i, n = 0;
    if (!s || max <= 0) return -1;
    for (i = 0; i < s->n && n < max; i++) {
        u32 hit = poll64_ready[s->fds[i]] & s->events[i];
        if (!hit) continue;
        if (fds_out) fds_out[n] = s->fds[i];
        if (ev_out) ev_out[n] = hit;
        n++;
    }
    return n;
}
