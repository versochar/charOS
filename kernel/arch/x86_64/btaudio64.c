/* 46F: Bluetooth audio — SBC yapilandirma + AVRCP + baglanti. */
#include "arch/x86_64/longmode.h"

#define BTAUDIO64_MAX_DEV 8

struct btaudio64_dev {
    int used;
    unsigned char mac[6];
    int connected;
};

static struct btaudio64_dev btaudio64_tab[BTAUDIO64_MAX_DEV];

/* SBC bit-hizi: ornekleme x kanal x bitpool x blok / (altbant*8). */
int btaudio64_sbc_config(u32 rate, int blocks, int subbands, int bitpool,
                         u32 *bitrate_out) {
    u64 br;
    if (!rate || rate > 48000) return -1;
    if (blocks != 4 && blocks != 8 && blocks != 12 && blocks != 16)
        return -2;
    if (subbands != 4 && subbands != 8) return -3;
    if (bitpool < 2 || bitpool > 250) return -4;
    br = (u64)rate * 2 * (u64)bitpool * (u64)blocks / ((u64)subbands * 8);
    if (bitrate_out) *bitrate_out = (u32)br;
    return 0;
}

/* AVRCP: 0=oynat,1=duraklat,2=sonraki,3=onceki,4=ses+-,5=sessiz */
int btaudio64_avrcp(int cmd, int *opcode_out) {
    static const int ops[] = {0x44, 0x45, 0x4B, 0x4C, 0x41, 0x43};
    if (cmd < 0 || cmd > 5) return -1;
    if (opcode_out) *opcode_out = ops[cmd];
    return 0;
}

int btaudio64_connect(const unsigned char mac[6]) {
    int i, k;
    if (!mac) return -1;
    for (i = 0; i < BTAUDIO64_MAX_DEV; i++) {
        int same = 1;
        if (!btaudio64_tab[i].used) continue;
        for (k = 0; k < 6; k++) {
            if (btaudio64_tab[i].mac[k] != mac[k]) {
                same = 0;
                break;
            }
        }
        if (same) {
            btaudio64_tab[i].connected = 1;
            return 0;
        }
    }
    for (i = 0; i < BTAUDIO64_MAX_DEV; i++) {
        if (!btaudio64_tab[i].used) {
            btaudio64_tab[i].used = 1;
            for (k = 0; k < 6; k++) btaudio64_tab[i].mac[k] = mac[k];
            btaudio64_tab[i].connected = 1;
            return 0;
        }
    }
    return -2;
}
