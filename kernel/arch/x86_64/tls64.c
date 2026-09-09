/* 43G: TLS 1.3 iskeleti — kayit basligi + el-sikisma durum makinesi.
 * Sifreleme 44x kriptoda dolar; burada cerceve + transkript karmasi (FNV).
 */
#include "arch/x86_64/longmode.h"

#define TLS64_HELLO_DONE 2
#define TLS64_KEY_EXCHANGE 3
#define TLS64_FINISHED 4

static int tls64_st = 0;
static u64 tls64_transcript_hash = 1469598103934665603ULL;

static void tls64_mix(const unsigned char *p, int len) {
    int i;
    for (i = 0; i < len; i++) {
        tls64_transcript_hash ^= p[i];
        tls64_transcript_hash *= 1099511628211ULL;
    }
}

int tls64_client_hello(unsigned char *out, int max) {
    /* Minimal ClientHello: kayit(5B) + hello govdesi (sabit+sifir suite) */
    int i;
    if (!out || max < 64) return -1;
    out[0] = 22; /* handshake */
    out[1] = 3;
    out[2] = 3; /* TLS 1.2 kayit, 1.3 iceride */
    out[3] = 0;
    out[4] = 48; /* boy */
    out[5] = 1;  /* client_hello */
    out[6] = 0;
    out[7] = 0;
    out[8] = 44;
    out[9] = 3;
    out[10] = 4; /* client 1.3 */
    for (i = 0; i < 32; i++) out[11 + i] = (unsigned char)(0xA0 + i);
    out[43] = 0; /* session id bos */
    out[44] = 0;
    out[45] = 2; /* 1 suite */
    out[46] = 0x13;
    out[47] = 0x01; /* TLS_AES_128_GCM_SHA256 */
    out[48] = 1;
    out[49] = 0; /* sikistirma yok */
    out[50] = 0;
    out[51] = 2; /* uzanti boy (bos) */
    tls64_mix(out, 53);
    tls64_st = TLS64_HELLO;
    return 53;
}

/* Sunucu kaydi: tur + durum ilerletme. Donus yeni durum. */
int tls64_process(const unsigned char *msg, int len) {
    if (!msg || len < 6) return -1;
    if (msg[0] != 22 && msg[0] != 20 && msg[0] != 21 && msg[0] != 23)
        return -2; /* bilinmeyen kayit */
    tls64_mix(msg, len > 64 ? 64 : len);
    if (tls64_st == TLS64_HELLO)
        tls64_st = TLS64_HELLO_DONE;
    else if (tls64_st == TLS64_HELLO_DONE)
        tls64_st = TLS64_KEY_EXCHANGE;
    else if (tls64_st == TLS64_KEY_EXCHANGE)
        tls64_st = TLS64_FINISHED;
    else if (tls64_st == TLS64_FINISHED)
        tls64_st = TLS64_ESTABLISHED;
    return tls64_st;
}

int tls64_state(void) { return tls64_st; }
u64 tls64_transcript(void) { return tls64_transcript_hash; }
