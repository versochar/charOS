/* 56C: ayar yoneticisi — kanal/anahtar deposu + degisim bildirimi. */
#include "arch/x86_64/longmode.h"

#define SETTINGS64_MAX 128

struct settings64_entry {
    int used;
    char channel[32];
    char key[48];
    char val[96];
};

struct settings64_watch {
    int used;
    char channel[32];
    char key[48];
};

static struct settings64_entry settings64_tab[SETTINGS64_MAX];
static struct settings64_watch settings64_watches[16];

#define SETTINGS64_MAXQ 16
struct settings64_change {
    char channel[32];
    char key[48];
};
static struct settings64_change settings64_q[SETTINGS64_MAXQ];
static int settings64_qhead = 0;
static int settings64_qn = 0;

/* Varsayilanlar: acik deger yoksa doner (ayri tablo, uzerine yazilmaz). */
static struct settings64_entry settings64_defs[SETTINGS64_MAX];

static void settings_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int settings_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct settings64_entry *settings_find(const char *ch,
                                              const char *key) {
    int i;
    for (i = 0; i < SETTINGS64_MAX; i++) {
        if (!settings64_tab[i].used) continue;
        if (!settings_str_eq(settings64_tab[i].channel, ch)) continue;
        if (settings_str_eq(settings64_tab[i].key, key))
            return &settings64_tab[i];
    }
    return 0;
}

static void settings64_fire(const char *ch, const char *key) {
    int i;
    for (i = 0; i < 16; i++) {
        int mc, kc;
        if (!settings64_watches[i].used) continue;
        mc = settings64_watches[i].channel[0] == 0 ||
             settings_str_eq(settings64_watches[i].channel, ch);
        kc = settings64_watches[i].key[0] == 0 ||
             settings_str_eq(settings64_watches[i].key, key);
        if (mc && kc) {
            int at;
            if (settings64_qn >= SETTINGS64_MAXQ) return; /* tasma: dusur */
            at = (settings64_qhead + settings64_qn) % SETTINGS64_MAXQ;
            settings_str_copy(settings64_q[at].channel, ch, 32);
            settings_str_copy(settings64_q[at].key, key, 48);
            settings64_qn++;
            return;
        }
    }
}

int settings64_set(const char *channel, const char *key, const char *val) {
    struct settings64_entry *e;
    int i;
    if (!channel || !key) return -1;
    e = settings_find(channel, key);
    if (e) {
        settings_str_copy(e->val, val ? val : "", 96);
        settings64_fire(channel, key);
        return 0;
    }
    for (i = 0; i < SETTINGS64_MAX; i++) {
        if (!settings64_tab[i].used) {
            settings64_tab[i].used = 1;
            settings_str_copy(settings64_tab[i].channel, channel, 32);
            settings_str_copy(settings64_tab[i].key, key, 48);
            settings_str_copy(settings64_tab[i].val, val ? val : "", 96);
            settings64_fire(channel, key);
            return 0;
        }
    }
    return -2;
}

static int settings64_get_raw(const char *channel, const char *key,
                              char *out, int max) {
    struct settings64_entry *e;
    if (!channel || !key || !out || max <= 0) return -1;
    e = settings_find(channel, key);
    if (!e) return -2;
    settings_str_copy(out, e->val, max);
    return 0;
}

/* Acik deger yoksa varsayilana duser (donus 1=varsayilan). */
int settings64_get(const char *channel, const char *key, char *out,
                   int max) {
    int i;
    if (settings64_get_raw(channel, key, out, max) == 0) return 0;
    for (i = 0; i < SETTINGS64_MAX; i++) {
        if (!settings64_defs[i].used) continue;
        if (!settings_str_eq(settings64_defs[i].channel, channel))
            continue;
        if (!settings_str_eq(settings64_defs[i].key, key)) continue;
        if (!out || max <= 0) return -1;
        settings_str_copy(out, settings64_defs[i].val, max);
        return 1;
    }
    return -2;
}

int settings64_set_int(const char *channel, const char *key,
                       long long v) {
    char tmp[24];
    int n = 0, neg = 0;
    unsigned long long u;
    if (v < 0) {
        neg = 1;
        u = (unsigned long long)(-(v + 1)) + 1ULL;
    } else {
        u = (unsigned long long)v;
    }
    if (!u)
        tmp[n++] = '0';
    while (u && n < 23) {
        tmp[n++] = (char)('0' + (u % 10));
        u /= 10;
    }
    if (neg && n < 23) tmp[n++] = '-';
    /* Ters cevir */
    {
        char buf[24];
        int i;
        for (i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
        buf[n] = 0;
        return settings64_set(channel, key, buf);
    }
}

int settings64_get_int(const char *channel, const char *key,
                       long long *out) {
    char buf[96];
    int i = 0, neg = 0;
    long long v = 0;
    if (settings64_get(channel, key, buf, sizeof(buf)) < 0) return -1;
    if (buf[0] == '-') {
        neg = 1;
        i = 1;
    }
    if (!buf[i]) return -2;
    for (; buf[i]; i++) {
        if (buf[i] < '0' || buf[i] > '9') return -2;
        v = v * 10 + (buf[i] - '0');
    }
    if (out) *out = neg ? -v : v;
    return 0;
}

int settings64_watch(const char *channel, const char *key) {
    int i;
    for (i = 0; i < 16; i++) {
        if (!settings64_watches[i].used) {
            settings64_watches[i].used = 1;
            settings_str_copy(settings64_watches[i].channel,
                              channel ? channel : "", 32);
            settings_str_copy(settings64_watches[i].key, key ? key : "",
                              48);
            return 0;
        }
    }
    return -1;
}

int settings64_changed(char *channel_out, int cmax, char *key_out,
                       int kmax) {
    if (!settings64_qn) return -1;
    if (channel_out && cmax > 0)
        settings_str_copy(channel_out, settings64_q[settings64_qhead].channel,
                          cmax);
    if (key_out && kmax > 0)
        settings_str_copy(key_out, settings64_q[settings64_qhead].key,
                          kmax);
    settings64_qhead = (settings64_qhead + 1) % SETTINGS64_MAXQ;
    settings64_qn--;
    return 0;
}

int settings64_set_bool(const char *channel, const char *key, int v) {
    return settings64_set(channel, key, v ? "true" : "false");
}

int settings64_get_bool(const char *channel, const char *key, int *out) {
    char buf[96];
    if (settings64_get(channel, key, buf, sizeof(buf)) < 0) return -1;
    if (!out) return -1;
    if (settings_str_eq(buf, "true") || settings_str_eq(buf, "1") ||
        settings_str_eq(buf, "yes") || settings_str_eq(buf, "on")) {
        *out = 1;
        return 0;
    }
    if (settings_str_eq(buf, "false") || settings_str_eq(buf, "0") ||
        settings_str_eq(buf, "no") || settings_str_eq(buf, "off")) {
        *out = 0;
        return 0;
    }
    return -2;
}

/* Ondalik: milli-birim tam sayi ("1.5" -> 1500). */
static int settings64_parse_milli(const char *s, long long *out) {
    long long v = 0, frac = 0, div = 1;
    int neg = 0, i = 0, f = 0;
    if (!s || !s[0]) return -1;
    if (s[0] == '-') {
        neg = 1;
        i = 1;
    }
    if (!s[i]) return -1;
    for (; s[i] && s[i] != '.'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        v = v * 10 + (s[i] - '0');
    }
    if (s[i] == '.') {
        i++;
        for (; s[i] && f < 3; i++, f++) {
            if (s[i] < '0' || s[i] > '9') return -1;
            frac = frac * 10 + (s[i] - '0');
            div *= 10;
        }
        if (s[i]) return -1; /* 3 haneden fazla */
        while (f++ < 3) frac *= 10;
    }
    v = v * 1000 + frac;
    if (out) *out = neg ? -v : v;
    return 0;
}

static void settings64_format_milli(long long m, char *out) {
    long long a;
    int pos = 0, i;
    char digits[24];
    int nd = 0;
    if (m < 0) {
        out[pos++] = '-';
        m = -m;
    }
    a = m / 1000;
    if (!a)
        digits[nd++] = '0';
    while (a && nd < 20) {
        digits[nd++] = (char)('0' + (a % 10));
        a /= 10;
    }
    for (i = nd - 1; i >= 0; i--) out[pos++] = digits[i];
    out[pos++] = '.';
    {
        long long f = m % 1000;
        out[pos++] = (char)('0' + (f / 100));
        out[pos++] = (char)('0' + ((f / 10) % 10));
        out[pos++] = (char)('0' + (f % 10));
    }
    out[pos] = 0;
}

int settings64_set_double(const char *channel, const char *key,
                          long long milli) {
    char buf[32];
    settings64_format_milli(milli, buf);
    return settings64_set(channel, key, buf);
}

int settings64_get_double(const char *channel, const char *key,
                          long long *milli_out) {
    char buf[96];
    if (settings64_get(channel, key, buf, sizeof(buf)) < 0) return -1;
    return settings64_parse_milli(buf, milli_out);
}

int settings64_reset(const char *channel, const char *key) {
    struct settings64_entry *e;
    if (!channel || !key) return -1;
    e = settings_find(channel, key);
    if (!e) return -2;
    e->used = 0;
    settings64_fire(channel, key);
    return 0;
}

int settings64_keys(const char *channel, char out[][48], int max) {
    int i, n = 0;
    if (!channel || !out || max <= 0) return -1;
    for (i = 0; i < SETTINGS64_MAX && n < max; i++) {
        int k;
        if (!settings64_tab[i].used) continue;
        if (!settings_str_eq(settings64_tab[i].channel, channel)) continue;
        for (k = 0; settings64_tab[i].key[k] && k < 47; k++)
            out[n][k] = settings64_tab[i].key[k];
        out[n][k] = 0;
        n++;
    }
    return n;
}

int settings64_channels(char out[][32], int max) {
    int i, n = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < SETTINGS64_MAX && n < max; i++) {
        int j, dup = 0, k;
        if (!settings64_tab[i].used) continue;
        for (j = 0; j < n; j++) {
            if (settings_str_eq(out[j], settings64_tab[i].channel)) {
                dup = 1;
                break;
            }
        }
        if (dup) continue;
        for (k = 0; settings64_tab[i].channel[k] && k < 31; k++)
            out[n][k] = settings64_tab[i].channel[k];
        out[n][k] = 0;
        n++;
    }
    return n;
}

int settings64_default(const char *channel, const char *key,
                       const char *val) {
    int i;
    if (!channel || !key) return -1;
    for (i = 0; i < SETTINGS64_MAX; i++) {
        if (settings64_defs[i].used &&
            settings_str_eq(settings64_defs[i].channel, channel) &&
            settings_str_eq(settings64_defs[i].key, key)) {
            settings_str_copy(settings64_defs[i].val, val ? val : "", 96);
            return 0;
        }
    }
    for (i = 0; i < SETTINGS64_MAX; i++) {
        if (!settings64_defs[i].used) {
            settings64_defs[i].used = 1;
            settings_str_copy(settings64_defs[i].channel, channel, 32);
            settings_str_copy(settings64_defs[i].key, key, 48);
            settings_str_copy(settings64_defs[i].val, val ? val : "", 96);
            return 0;
        }
    }
    return -2;
}

/* "kanal.anahtar=deger\n" bicimi; donus yazilan bayt. */
int settings64_export(char *out, int max) {
    int i, pos = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < SETTINGS64_MAX; i++) {
        int k;
        struct settings64_entry *e = &settings64_tab[i];
        if (!e->used) continue;
        for (k = 0; e->channel[k] && pos + 1 < max; k++)
            out[pos++] = e->channel[k];
        if (pos + 1 >= max) return -2;
        out[pos++] = '.';
        for (k = 0; e->key[k] && pos + 1 < max; k++)
            out[pos++] = e->key[k];
        if (pos + 1 >= max) return -2;
        out[pos++] = '=';
        for (k = 0; e->val[k] && pos + 1 < max; k++)
            out[pos++] = e->val[k];
        if (pos + 1 >= max) return -2;
        out[pos++] = '\n';
    }
    if (pos < max)
        out[pos] = 0;
    else
        return -2;
    return pos;
}

int settings64_import(const char *buf) {
    const char *p;
    if (!buf) return -1;
    p = buf;
    while (*p) {
        char ch[32], key[48], val[96];
        int i = 0;
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;
        if (*p == '#') {
            while (*p && *p != '\n') p++;
            continue;
        }
        while (*p && *p != '.' && *p != '=' && *p != '\n' && i < 31)
            ch[i++] = *p++;
        ch[i] = 0;
        if (*p != '.') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        p++;
        i = 0;
        while (*p && *p != '=' && *p != '\n' && i < 47)
            key[i++] = *p++;
        key[i] = 0;
        if (*p != '=') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        p++;
        i = 0;
        while (*p && *p != '\n' && *p != '\r' && i < 95)
            val[i++] = *p++;
        val[i] = 0;
        while (*p && *p != '\n') p++;
        if (*p) p++;
        if (!ch[0] || !key[0]) continue;
        settings64_set(ch, key, val);
    }
    return 0;
}
