#ifndef USER_DNS_H
#define USER_DNS_H

/* 14I: DNS istemci yardımcısı (saf fonksiyonlar, ağsız test edilebilir).
 * Sorgu kurar, yanıtı ayrıştırır (sıkıştırma işaretçili isimler dahil). */

int dns_build_query(const char* name, unsigned short txid,
                    char* out, int maxlen); /* dönüş: bayt / -1 */
int dns_parse_response(const char* msg, int len,
                       unsigned char ip[4]); /* ilk A kaydı, 0 ok */

#endif
