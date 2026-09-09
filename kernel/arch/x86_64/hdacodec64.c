/* 46A: HD Audio codec — komut kurma + yanit cozumu + grup tarama. */
#include "arch/x86_64/longmode.h"

u32 hdacodec64_verb(u8 nid, u16 verb, u16 param) {
    return ((u32)nid << 28) | ((u32)(verb & 0xFFF) << 8) | param;
}

/* Pin config default: bit31-30=port, 29-28=loc, 23-20=dev,
 * 19-16=conn, 15-12=color. Ornek 0x00771000: loc=0, dev=7, conn=1. */
int hdacodec64_pin_config(u32 resp, int *loc, int *dev, int *conn,
                          int *color) {
    if (loc) *loc = (int)((resp >> 28) & 0x3);
    if (dev) *dev = (int)((resp >> 20) & 0xF);
    if (conn) *conn = (int)((resp >> 16) & 0xF);
    if (color) *color = (int)((resp >> 12) & 0xF);
    return 0;
}

/* Widget turu: param bit23-20 (0=out,1=in,2=mixer,3=selector,4=pin...). */
int hdacodec64_widget_type(u32 param) {
    return (int)((param >> 20) & 0xF);
}

/* Grup yanitlarindan NID listesi (baslangic+adet formati). */
int hdacodec64_fg_scan(const u32 *resps, int n, u8 *nids, int max) {
    int i, total = 0;
    if (!resps || n <= 0 || !nids || max <= 0) return -1;
    for (i = 0; i < n && total < max; i++) {
        u32 start = resps[i] & 0xFF;
        u32 count = (resps[i] >> 8) & 0xFF;
        u32 k;
        if (!count || !start) continue;
        for (k = 0; k < count && total < max; k++)
            nids[total++] = (u8)(start + k);
    }
    return total;
}
