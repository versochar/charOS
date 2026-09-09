/* 44I: surucu oz-test — register + geri-dongu + firmware nabzi. */
#include "arch/x86_64/longmode.h"

int drvtest64_regs(u32 id_val) {
    /* Beklenen kimlik: DID 0xC821 ust 16 bitte */
    if ((id_val >> 16) != 0xC821) return -1;
    if (!(id_val & 0x1)) return -2; /* MMIO biti sart */
    return 0;
}

int drvtest64_loopback(const void *frame, u64 len) {
    unsigned char tmp[1600];
    u64 i;
    int ac = MAC11_AC_BE;
    if (!frame || !len || len > 1600) return -1;
    if (mac11_tx(ac, frame, len) != 0) return -2;
    if (mac11_rx(ac, tmp, sizeof(tmp)) != (int)len) return -3;
    for (i = 0; i < len; i++) {
        if (tmp[i] != ((const unsigned char *)frame)[i]) return -4;
    }
    return 0;
}

int drvtest64_fw_alive(u32 heartbeat) {
    static u32 last = 0;
    static int ticks = 0;
    if (heartbeat != last) {
        last = heartbeat;
        ticks = 0;
        return 0; /* ilerliyor */
    }
    if (++ticks > 100) return -1; /* takildi */
    return 1; /* henuz karar yok */
}
