/* 14I: minimal HTTP/1.0 istemci yardımcısı. */
#include "http.h"

static int strapp(char* out, int max, int pos, const char* s) {
    while (*s) {
        if (pos >= max - 1) return -1;
        out[pos++] = *s++;
    }
    return pos;
}

int http_build_get(const char* host, const char* path,
                   char* out, int maxlen) {
    if (!host || !path || !out || maxlen < 32) return -1;
    int pos = 0;
    pos = strapp(out, maxlen, pos, "GET ");
    if (pos < 0) return -1;
    pos = strapp(out, maxlen, pos, path);
    if (pos < 0) return -1;
    pos = strapp(out, maxlen, pos, " HTTP/1.0\r\nHost: ");
    if (pos < 0) return -1;
    pos = strapp(out, maxlen, pos, host);
    if (pos < 0) return -1;
    pos = strapp(out, maxlen, pos, "\r\n\r\n");
    if (pos < 0) return -1;
    out[pos] = 0;
    return pos;
}

int http_parse_status(const char* line, int len) {
    /* "HTTP/1.x NNN ..." */
    if (!line || len < 12) return -1;
    if (line[0] != 'H' || line[1] != 'T' || line[2] != 'T' ||
        line[3] != 'P' || line[4] != '/')
        return -1;
    int i = 5;
    while (i < len && line[i] != ' ') i++;
    if (i >= len) return -1;
    i++;
    if (i + 2 >= len) return -1;
    if (line[i] < '0' || line[i] > '9' ||
        line[i + 1] < '0' || line[i + 1] > '9' ||
        line[i + 2] < '0' || line[i + 2] > '9')
        return -1;
    return (line[i] - '0') * 100 + (line[i + 1] - '0') * 10 +
           (line[i + 2] - '0');
}

static int find_crlfcrlf(const char* m, int len) {
    for (int i = 0; i + 3 < len; i++)
        if (m[i] == '\r' && m[i + 1] == '\n' && m[i + 2] == '\r' &&
            m[i + 3] == '\n')
            return i;
    return -1;
}

/* Başlıklardan Content-Length tara (yoksa -1) */
static int content_length(const char* m, int hlen) {
    const char* key = "Content-Length:";
    int kl = 15;
    for (int i = 0; i + kl < hlen; i++) {
        int k = 0;
        while (k < kl && m[i + k] == key[k]) k++;
        if (k < kl) continue;
        int j = i + kl;
        while (j < hlen && (m[j] == ' ' || m[j] == '\t')) j++;
        int v = 0, any = 0;
        while (j < hlen && m[j] >= '0' && m[j] <= '9') {
            v = v * 10 + (m[j] - '0');
            j++;
            any = 1;
        }
        if (any) return v;
        return -1;
    }
    return -1;
}

int http_parse_response(const char* msg, int len, int* status,
                        int* body_off, int* body_len) {
    if (!msg || len <= 0) return -1;
    int hs = find_crlfcrlf(msg, len);
    if (hs < 0) return -1;
    int eol = 0;
    while (eol + 1 < len && !(msg[eol] == '\r' && msg[eol + 1] == '\n')) eol++;
    int st = http_parse_status(msg, eol);
    if (st < 0) return -1;
    int bo = hs + 4;
    int cl = content_length(msg, hs);
    int bl = (cl >= 0) ? cl : (len - bo);
    if (bo + bl > len) bl = len - bo;
    if (bl < 0) bl = 0;
    if (status) *status = st;
    if (body_off) *body_off = bo;
    if (body_len) *body_len = bl;
    return 0;
}
