#ifndef USER_HTTP_H
#define USER_HTTP_H

/* 14I: minimal HTTP istemci yardımcısı (saf fonksiyonlar).
 * GET kurar, yanıtı ayrıştırır (durum + Content-Length gövdesi). */

int http_build_get(const char* host, const char* path,
                   char* out, int maxlen); /* dönüş: bayt / -1 */
int http_parse_status(const char* line, int len); /* 200 vb. / -1 */
int http_parse_response(const char* msg, int len, int* status,
                        int* body_off, int* body_len); /* 0 ok */

#endif
