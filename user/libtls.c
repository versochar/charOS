/* 15C: TLS (thread-local storage). -ftls-model=global-dynamic kullanır:
 * erişim ___tls_get_addr (yükleyici exportu) üzerinden per-thread bloğa gider. */
__thread int g_tls;

int tls_set(int v) { g_tls = v; return v; }
int tls_get(void) { return g_tls; }
int tls_inc(void) { g_tls++; return g_tls; }