/* 34E: NVRAM boot girdisi — Boot#### adi + oznitelik dogrulama.
 * SetVariable cagrisi loader'da BS ile; burada saf uretim/mantik.
 */
#include "arch/x86_64/longmode.h"

int nvram64_boot_name(u32 idx, char out[9]) {
    static const char hex[] = "0123456789ABCDEF";
    if (idx > 0xFFFF || !out) return -1;
    out[0] = 'B';
    out[1] = 'o';
    out[2] = 'o';
    out[3] = 't';
    out[4] = hex[(idx >> 12) & 0xF];
    out[5] = hex[(idx >> 8) & 0xF];
    out[6] = hex[(idx >> 4) & 0xF];
    out[7] = hex[idx & 0xF];
    out[8] = 0;
    return 0;
}

int nvram64_attrs_valid(u32 attrs) {
    /* Boot girdisi NV+BS+RT tasimalidir; baska bitler gelecekte genisler. */
    if (!(attrs & (u32)NVRAM64_NV)) return -1;
    if (!(attrs & (u32)NVRAM64_BS)) return -2;
    if (!(attrs & (u32)NVRAM64_RT)) return -3;
    return 0;
}
